
#include <call_graph.h>
CallNode::CallNode(CallFunc *caller, llvm::Instruction *inst) {
    this->caller = caller;
    this->inst = inst;
}

CallFunc *CallNode::get_caller() {
    return this->caller;
}

std::vector<CallFunc *> &CallNode::get_callees() {
    return this->callees;
}

void CallNode::add_callee(CallFunc *callee) {
    this->callees.push_back(callee);
}

llvm::Instruction *CallNode::get_inst() {
    return this->inst;
}

void CallNode::dump() {
    unsigned line = this->inst->getDebugLoc().getLine();
    llvm::errs() << "    " << line << ": ";
    this->inst->dump();
    for (auto &callee : this->callees) {
        llvm::errs() << "        " << callee->get_func()->getName() << "\n";
    }
}

CallNode::~CallNode() {
}
CallFunc::CallFunc(llvm::Function *func) {
    this->func = func;
}

llvm::Function *CallFunc::get_func() {
    return this->func;
}
void CallFunc::dump() {
    llvm::errs() << this->func->getName() << ":\n";
    for (auto &pair : this->call_nodes)
        pair.second->dump();
}
void CallFunc::add_call_node(CallNode *call_node) {
    this->call_nodes[call_node->get_inst()] = call_node;
}
std::map<Instruction *, CallNode *> &CallFunc::get_call_nodes() {
    return this->call_nodes;
}

CallFunc::~CallFunc() {
    for (auto pair : this->call_nodes)
        delete pair.second;
}

Call_Graph::Call_Graph() {
    for (auto &pair : this->callfuncs)
        delete pair.second;
}

CallFunc *Call_Graph::get_callfunc(Function *func) {
    return this->callfuncs[func];
}

void Call_Graph::dump() {
    for (auto &pair : this->callfuncs) {
        pair.second->dump();
    }
}

void Call_Graph::add_call_func(CallFunc *callfunc) {
    this->callfuncs[callfunc->get_func()] = callfunc;
}
std::map<Function *, CallFunc *> &Call_Graph::get_call_funcs() {
    return this->callfuncs;
}
Call_Graph::~Call_Graph() {
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
static std::map<CallFunc *, std::set<llvm::Function *>> RETs;
static bool insert_may_value(Value *var, Value *value) {
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
static bool insert_may_value(Value *var, CallFunc *callee) {
    assert(RETs.find(callee) != RETs.end());
    auto old = PVVs[var];
    PVVs[var].insert(RETs[callee].begin(), RETs[callee].end());
    return old != PVVs[var];
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

static bool deal_inst(Call_Graph &callgraph, CallFunc *callfunc, Instruction *inst) {
    // inst->dump();
    bool change = false;
    if (PHINode *phiInst = llvm::dyn_cast<llvm::PHINode>(inst)) {
        unsigned num = phiInst->getNumIncomingValues();
        for (unsigned i = 0; i < num; i++) {
            Value *op = phiInst->getIncomingValue(i);
            change |= insert_may_value(phiInst, op);
        }
        return change;
    }

    if (CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(inst)) {
        if (inst->getDebugLoc().getLine() == 0) return false;
        // assert(!callInst->getType()->isPointerTy()); // 返回值先不是指针
        Value *called = callInst->getCalledOperand();
        CallNode *callnode = new CallNode(callfunc, inst);
        if (auto *func = llvm::dyn_cast<llvm::Function>(called)) {
            callnode->add_callee(callgraph.get_callfunc(func));
        }
        else {
            // llvm::errs() << called << "\n";
            for (auto &func : PVVs[called]) {
                callnode->add_callee(callgraph.get_callfunc(func));
            }
        }
        callfunc->add_call_node(callnode);
        if (is_func_ptr(callInst)) {
            for (auto &callee : callnode->get_callees()) {
                change |= insert_may_value(callInst, callee);
            }
        }
        // callInst->dump();
        for (unsigned i = 0; i < callInst->getNumArgOperands(); i++) {
            Value *arg = callInst->getArgOperand(i);
            if (arg->getType()->isPointerTy() && arg->getType()->getPointerElementType()->isFunctionTy()) {
                for (auto &callee : callnode->get_callees()) {
                    Argument *argument = callee->get_func()->getArg(i);
                    // arg->dump();
                    // argument->dump();
                    // llvm::errs() << argument << "\n";
                    change |= insert_may_value(argument, arg);
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
}

bool Build_Call_Graph_Pass::runOnModule(Module &M) {
    std::map<CallFunc *, std::vector<BasicBlock *>> rpo_list;
    for (auto &F : M) {
        if (F.getName().startswith("llvm.dbg")) continue;
        CallFunc *callfunc = new CallFunc(&F);
        RETs[callfunc] = std::set<llvm::Function *>();
        this->call_graph.add_call_func(callfunc);
        std::unordered_map<BasicBlock *, bool> visited;
        reverse_post_order(visited, rpo_list[callfunc], &(F.getEntryBlock()));
    }
    bool change = true;
    while (change) {
        change = false;
        for (auto it = M.rbegin(); it != M.rend(); it++) {
            Function &F = *it;
            if (F.getName().startswith("llvm.dbg")) continue;
            CallFunc *callfunc = this->call_graph.get_callfunc(&F);
            // F.dump();
            for (auto iter = rpo_list[callfunc].rbegin(); iter != rpo_list[callfunc].rend(); iter++) {
                BasicBlock *BB = *iter;
                // BB->dump();
                for (auto &I : BB->getInstList()) {
                    change |= deal_inst(this->call_graph, callfunc, &I);
                }
            }
        }
    }
    return false;
}

char Build_Call_Graph_Pass::ID = 0;
static RegisterPass<Build_Call_Graph_Pass> X("my-analysis-pass", "My Analysis Pass", false, true);