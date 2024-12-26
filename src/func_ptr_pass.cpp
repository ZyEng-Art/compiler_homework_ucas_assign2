#include <call_graph.h>
#include <func_ptr_pass.h>
#include <stack>
#include <utils.h>

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
///Updated 11/10/2017 by fargo: make all functions
///processed by mem2reg before this pass.

bool FuncPtrPass::can_execuate(llvm::PHINode phiInstr, llvm::Value *operand, llvm::BasicBlock instr_block) {
    // TODO: （测例并不强求）
    // 分析 phi 指令取operand（operand是常数）时能否执行到 instr_block
    // 这个分析是稀疏条件常量传播所不能化简的，
    // 即：条件判断的条件是phi,虽然化简不了条件跳转
    // 但是可以分析出一个变量可能取哪些值，取某个phi后之后的某个分支就可能不会执行
    return true;
}

#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/CFG.h>
#include <unordered_map>
static void reverse_post_order(std::unordered_map<BasicBlock *, bool> &visited, std::vector<BasicBlock *> &rpo_list, BasicBlock *entry) {
    if (visited.find(entry) != visited.end()) return;
    visited[entry] = true;
    for (auto *succ : successors(entry)) {
        reverse_post_order(visited, rpo_list, succ);
    }
    rpo_list.push_back(entry);
}
static std::map<llvm::Value *, std::set<llvm::Function *>> PVVs;
static std::map<Function *, std::set<llvm::Function *>> RETs;
static bool insert_may_value1(Value *var, Value *value) {
    if (auto *func = llvm::dyn_cast<llvm::Function>(value)) {
        if (PVVs[var].find(func) == PVVs[var].end()) {
            PVVs[var].insert(func);
            return true;
        }
    }
    else {
        auto old = PVVs[var];
        PVVs[var].insert(PVVs[value].begin(), PVVs[value].end());
        return old != PVVs[var];
    }
    return false;
}
static bool insert_may_value2(Value *var, Function *callee) {
    assert(RETs.find(callee) != RETs.end());
    auto old = PVVs[var];
    PVVs[var].insert(RETs[callee].begin(), RETs[callee].end());
    return old != PVVs[var];
}

static bool is_dbg_func(Function *F) {
    return F->getName().startswith("llvm.dbg");
}
static bool is_func_ptr(Value *value) {
    if (llvm::PointerType *ptrType = llvm::dyn_cast<llvm::PointerType>(value->getType())) {
        // 获取指针类型的元素类型
        llvm::Type *elementType = ptrType->getElementType();

        // 判断元素类型是否是函数类型
        return elementType->isFunctionTy();
    }
    return false;
}

static bool deal_inst(Function *callfunc, Instruction *inst) {
    bool change = false;
    if (PHINode *phiInst = llvm::dyn_cast<llvm::PHINode>(inst)) {
        unsigned num = phiInst->getNumIncomingValues();
        for (unsigned i = 0; i < num; i++) {
            Value *op = phiInst->getIncomingValue(i);
            change |= insert_may_value1(phiInst, op);
        }
        return change;
    }

    if (CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(inst)) {
        if (inst->getDebugLoc().getLine() == 0) return false;

        Value *called = callInst->getCalledOperand();
        // 更新返回值和当前值
        if (auto *func = llvm::dyn_cast<llvm::Function>(called)) {
            change |= insert_may_value1(called, func);
            change |= insert_may_value2(callInst, func);
        }
        else {
            for (auto &func : PVVs[called]) {
                change |= insert_may_value2(callInst, func);
            }
        }
        // 更新参数
        for (unsigned i = 0; i < callInst->getNumArgOperands(); i++) {
            Value *arg = callInst->getArgOperand(i);
            if (arg->getType()->isPointerTy() && arg->getType()->getPointerElementType()->isFunctionTy()) {
                if (auto *func = llvm::dyn_cast<llvm::Function>(called)) {
                    Argument *argument = func->getArg(i);
                    change |= insert_may_value1(argument, arg);
                }
                else {
                    for (auto &callee : PVVs[called]) {
                        Argument *argument = callee->getArg(i);
                        change |= insert_may_value1(argument, arg);
                    }
                }
            }
        }
        return change;
    }

    if (ReturnInst *returnInst = dyn_cast<ReturnInst>(inst)) {
        Value *ret = returnInst->getReturnValue();
        if (is_func_ptr(ret)) {
            if (RETs[callfunc] == PVVs[ret])
                return false;
            RETs[callfunc] = PVVs[ret];
            return true;
        }
    }
    return false;
}

bool FuncPtrPass::runOnModule(Module &M) {
    std::map<Function *, std::vector<BasicBlock *>> rpo_list;
    for (auto &F : M) {
        if (F.getName().startswith("llvm.dbg")) continue;
        RETs[&F] = std::set<llvm::Function *>();
        std::unordered_map<BasicBlock *, bool> visited;
        reverse_post_order(visited, rpo_list[&F], &(F.getEntryBlock()));
    }
    bool change = true;
    while (change) {
        change = false;
        for (auto it = M.rbegin(); it != M.rend(); it++) {
            Function &F = *it;
            if (is_dbg_func(&F)) continue;
            for (auto iter = rpo_list[&F].rbegin(); iter != rpo_list[&F].rend(); iter++) {
                BasicBlock *BB = *iter;
                for (auto &I : BB->getInstList()) {
                    change |= deal_inst(&F, &I);
                }
            }
        }
    }

    for (auto &F : M) {
        if (is_dbg_func(&F)) continue;
        for (auto &BB : F) {
            for (auto &I : BB.getInstList()) {
                if (CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    if (I.getDebugLoc().getLine() == 0) continue;
                    bool first = true;
                    llvm::errs() << callInst->getDebugLoc().getLine() << " :";
                    for (auto &callee : PVVs[callInst->getCalledOperand()]) {
                        if (first) {
                            llvm::errs() << " ";
                            first = false;
                        }
                        else {
                            llvm::errs() << ", ";
                        }
                        llvm::errs() << callee->getName();
                    }
                    llvm::errs() << "\n";
                }
            }
        }
    }
    return false;
}

// llvm.dbg.value 没用
char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");
// 注册Pass
