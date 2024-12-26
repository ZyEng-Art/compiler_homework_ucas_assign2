#ifndef __FUNC_PTR_PASS__
#define __FUNC_PTR_PASS__
#include <utils.h>

struct FuncPtrPass : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    FuncPtrPass()
        : ModulePass(ID) { }
    bool can_execuate(llvm::PHINode phiInstr, llvm::Value *operand, llvm::BasicBlock instr_block);
    bool runOnModule(Module &M) override;
};

#endif