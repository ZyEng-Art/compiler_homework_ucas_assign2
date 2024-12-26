
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
