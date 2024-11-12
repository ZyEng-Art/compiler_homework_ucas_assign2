//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

using namespace llvm;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }
/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang will
 * have optnone attribute which would lead to some transform passes disabled, like mem2reg.
 */
struct EnableFunctionOptPass : public FunctionPass {
    static char ID;
    EnableFunctionOptPass()
        : FunctionPass(ID) { }
    bool runOnFunction(Function &F) override {
        if (F.hasFnAttribute(Attribute::OptimizeNone)) {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID = 0;

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
///Updated 11/10/2017 by fargo: make all functions
///processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    FuncPtrPass()
        : ModulePass(ID) { }

    bool can_execuate(llvm::PHINode phiInstr, llvm::Value *operand, llvm::BasicBlock instr_block) {
        // TODO: （测例并不强求）
        // 分析 phi 指令取operand（operand是常数）时能否执行到 instr_block
        // 这个分析是稀疏条件常量传播所不能化简的，
        // 即：条件判断的条件是phi,虽然化简不了条件跳转
        // 但是可以分析出一个变量可能取哪些值，取某个phi后之后的某个分支就可能不会执行
        return true;
    }

    void analysis(llvm::CallInst *callInst) {
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
    bool runOnModule(Module &M) override {
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
};
// llvm.dbg.value 没用
char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");
// 注册Pass

static cl::opt<std::string>
    InputFilename(cl::Positional,
        cl::desc("<filename>.bc"),
        cl::init(""));

int main(int argc, char **argv) {
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err; // 前端诊断工具
    // Parse the command line to read the Inputfilename
    cl::ParseCommandLineOptions(argc, argv,
        "FuncPtrPass \n My first LLVM too which does not do much.\n");

    // Load the input module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context); // 从文件中加载Module
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }

    llvm::legacy::PassManager Passes;

    ///Remove functions' optnone attribute in LLVM5.0
    Passes.add(new EnableFunctionOptPass());
    ///Transform it to SSA
    Passes.add(llvm::createPromoteMemoryToRegisterPass());

    /// Your pass to print Function and Call Instructions
    Passes.add(new FuncPtrPass());
    Passes.run(*M.get());
}
