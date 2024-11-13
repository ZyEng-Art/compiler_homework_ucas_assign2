#ifndef __UTILS__
#define __UTILS__

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
using namespace llvm;
// 没有这两个不能正常遍历模块
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#include <llvm/Support/raw_ostream.h>
#include <vector>

#endif