#ifndef __CALL_GRAPH__
#define __CALL_GRAPH__

#include <utils.h>

class CallFunc;
class CallNode {
private:
    CallFunc *caller;
    std::vector<CallFunc *> callees;
    llvm::Instruction *inst;

public:
    // 创建callnode时就将其加入caller
    CallNode(CallFunc *caller, llvm::Instruction *inst);
    CallFunc *get_caller();
    std::vector<CallFunc *> &get_callees();
    void add_callee(CallFunc *callee);
    llvm::Instruction *get_inst();
    void dump();
    ~CallNode();
};

class CallFunc {
private:
    llvm::Function *func;
    std::map<Instruction *, CallNode *> call_nodes;

public:
    CallFunc(llvm::Function *func);
    llvm::Function *get_func();
    void add_call_node(CallNode *call_node);
    void dump();
    std::map<Instruction *, CallNode *> &get_call_nodes();
    ~CallFunc();
};
class Call_Graph {
private:
    std::map<Function *, CallFunc *> callfuncs;

public:
    Call_Graph();
    void add_call_func(CallFunc *callfunc);
    void dump();
    CallFunc *get_callfunc(Function *func);
    std::map<Function *, CallFunc *> &get_call_funcs();
    ~Call_Graph();
};

#endif