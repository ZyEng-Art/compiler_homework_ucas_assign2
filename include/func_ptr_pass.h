#ifndef __FUNC_PTR_PASS__
#define __FUNC_PTR_PASS__
#include <utils.h>

struct FuncPtrPass : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    FuncPtrPass()
        : ModulePass(ID) { }
    bool can_execuate(llvm::PHINode phiInstr, llvm::Value *operand, llvm::BasicBlock instr_block);
    void analysis(llvm::CallInst *callInst);
    bool runOnModule(Module &M) override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;
};

#endif