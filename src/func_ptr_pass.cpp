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

bool FuncPtrPass::runOnModule(Module &M) {
    Call_Graph &call_graph = getAnalysis<Build_Call_Graph_Pass>().call_graph;
    // 遍历模块中的每个函数
    for (auto &F : M) {
        if (F.getName().startswith("llvm.dbg")) continue;
        CallFunc *callfunc = call_graph.get_callfunc(&F);
        for (auto pair : callfunc->get_call_nodes()) {
            Instruction *instr = pair.first;
            llvm::errs() << instr->getDebugLoc().getLine() << " :";
            bool first = true;
            for (CallFunc *callee : pair.second->get_callees()) {
                if (first) {
                    llvm::errs() << " ";
                    first = false;
                }
                else {
                    llvm::errs() << ", ";
                }
                llvm::errs() << callee->get_func()->getName();
            }
            llvm::errs() << "\n";
        }
    }
    return false;
}

void FuncPtrPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<Build_Call_Graph_Pass>();
    AU.setPreservesAll();
}
// llvm.dbg.value 没用
char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");
// 注册Pass
