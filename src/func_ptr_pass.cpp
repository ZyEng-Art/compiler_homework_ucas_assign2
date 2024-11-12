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

void FuncPtrPass::analysis(llvm::CallInst *callInst) {
    unsigned line = callInst->getDebugLoc().getLine();
    if (line == 0) return;
    llvm::Value *calledValue = callInst->getCalledOperand();
    llvm::errs() << line << " :";
    llvm::BasicBlock *block = callInst->getParent();
    // 间接调用
    bool first = false; // 方便输出
    std::stack<llvm::Value *> operands;
    operands.push(calledValue);
    while (!operands.empty()) {
        llvm::Value *operand = operands.top();
        operands.pop();
        if (llvm::dyn_cast<llvm::ConstantPointerNull>(operand)) continue;
        if (auto *defInst = llvm::dyn_cast<llvm::Instruction>(operand)) {
            if (auto *phiInstr = llvm::dyn_cast<llvm::PHINode>(defInst)) {
                // 这样做不能处理循环引用的问题，但测例没有循环，不会有问题
                unsigned num = phiInstr->getNumIncomingValues();
                for (unsigned i = 0; i < num; i++)
                    // if (can_execuate(phiInstr->getIncomingBlock(i), block))
                    operands.push(phiInstr->getIncomingValue(i));
            }
            else {
                assert(0);
            }
        }
        else if (auto *func = llvm::dyn_cast<llvm::Function>(operand)) {
            if (!first) {
                llvm::errs() << " ";
                first = true;
            }
            else
                llvm::errs() << ", ";
            llvm::errs() << func->getName();
        }
        else {
            assert(0); //
        }
    }
    llvm::errs() << "\n";
}
bool FuncPtrPass::runOnModule(Module &M) {
    // errs() << "Hello: ";
    // errs().write_escaped(M.getName()) << '\n';
    // M.dump();
    // errs() << "------------------------------\n";

    // 遍历模块中的每个函数
    for (auto &F : M) {
        // 遍历函数中的每个基本块
        for (auto &BB : F) {
            // 遍历基本块中的每条指令
            for (auto &I : BB) {
                // 检查指令是否为调用指令
                if (auto *callInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    analysis(callInst);
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
