//===-- obfuscate/obfuscate.cpp - main file of obfuscate_string -*- C++ -*-===//
//
// The main file of the obfusate_string pass.
// LLVM project is under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the module pass class and decrypt function.
///
//===----------------------------------------------------------------------===//
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <map>
#include <vector>

using namespace llvm;

namespace {
/// encrypt strings with xor.
/// \param s input string for encrypt.
void encrypt(std::string &s) {
  for (int i = 0; i < s.length() - 1; i++) {
    s[i] ^= 42;
  }
}

/// A pass for obfuscating const string in modules.
struct ObfuscatePass : public ModulePass {
  static char ID;
  ObfuscatePass() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    for (GlobalValue &GV : M.globals()) {
      GlobalVariable *GVar = dyn_cast<GlobalVariable>(&GV);
      if (GVar == nullptr) {
        continue;
      }
      std::vector<std::pair<Instruction *, User *>> Target;
      bool hasExceptCallInst = false;
      // Find all user and encrypt const value, then insert decrypt function.
      for (User *Usr : GVar->users()) {
        // Get instruction.
        Instruction *Inst = dyn_cast<Instruction>(Usr);
        if (Inst == nullptr) {
          // If Usr is not an instruction, like i8* getelementptr...
          // Dig deeper to find Instruction.
          for (User *DirecUsr : Usr->users()) {
            Inst = dyn_cast<Instruction>(DirecUsr);
            if (Inst == nullptr) {
              continue;
            }
            if (!isa<CallInst>(Inst)) {
              hasExceptCallInst = true;
              Target.clear();
            } else {
              Target.emplace_back(std::pair<Instruction *, User *>(Inst, Usr));
            }
          }
        }
      }
      if (hasExceptCallInst == false && Target.size() == 1) {
        for (auto &T : Target) {
          obfuscateString(M, T.first, T.second, GVar);
        }
        // Change constant to variable.
        GVar->setConstant(false);
      }
    }
    return true;
  }

  /// Obfuscate string and add decrypt function.
  /// \param M Module
  /// \param Inst Instruction
  /// \param Usr User of the \p GlobalVariable
  /// \param GVar The const string
  void obfuscateString(Module &M, Instruction *Inst, Value *Usr,
                       GlobalVariable *GVar) {
    // Encrypt origin string and replace it encrypted string.
    ConstantDataArray *GVarArr =
        dyn_cast<ConstantDataArray>(GVar->getInitializer());
    if (GVarArr == nullptr) {
      return;
    }
    std::string Origin;
    if (GVarArr->isString(8)) {
      Origin = GVarArr->getAsString().str();
    } else if (GVarArr->isCString()) {
      Origin = GVarArr->getAsCString().str();
    }
    encrypt(Origin);
    Constant *NewConstStr = ConstantDataArray::getString(
        GVarArr->getContext(), StringRef(Origin), false);
    GVarArr->replaceAllUsesWith(NewConstStr);

    // Insert decrypt function above Inst with IRBuilder.
    IRBuilder<> builder(Inst);
    Type *Int8PtrTy = builder.getInt8PtrTy();
    Type *Int64Ty = builder.getInt64Ty();
    // Create decrypt function in GlobalValue / Get decrypt function.
    SmallVector<Type *, 1> FuncArgs = {Int8PtrTy, Int64Ty};
    FunctionType *FuncType = FunctionType::get(Int8PtrTy, FuncArgs, false);
    FunctionCallee DecryptFunc = M.getOrInsertFunction("__decrypt", FuncType);
    FunctionCallee EncryptFunc = M.getOrInsertFunction("__encrypt", FuncType);
    ConstantInt *StringLength = builder.getInt64(Origin.length() - 1);
    // Create call instrucions.
    SmallVector<Value *, 2> CallArgs = {Usr, StringLength};
    CallInst *DecryptInst =
        builder.CreateCall(FuncType, DecryptFunc.getCallee(), CallArgs);
    CallInst *EncryptInst =
        CallInst::Create(FuncType, EncryptFunc.getCallee(), CallArgs);
    EncryptInst->insertAfter(Inst);
  }
};
} // namespace

char ObfuscatePass::ID = 0;
static RegisterPass<ObfuscatePass> X("obfstr", "obfuscate string");