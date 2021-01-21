#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"

#include <random>

using namespace llvm;

static cl::opt<int>
    ObfTimes("sub_loop",
             cl::desc("Choose how many time the -sub pass loops on a function"),
             cl::value_desc("number of times"), cl::init(2), cl::Optional);

static cl::opt<unsigned int>
    ObfProbRate("sub_prob",
                cl::desc("Choose the probability [%] each basic blocks will be "
                         "obfuscated by the InstructioSubstitution pass"),
                cl::value_desc("probability rate"), cl::init(50), cl::Optional);

struct SubstitutionPass : public FunctionPass {
  static char ID;
  // Store the substitution function
  SmallVector<void (SubstitutionPass::*)(Instruction *), 3> addFunc;
  SmallVector<void (SubstitutionPass::*)(Instruction *), 2> subFunc;
  std::mt19937 rng;

  SubstitutionPass() : FunctionPass(ID), rng(std::random_device{}()) {
    addFunc = {&SubstitutionPass::addNeg, &SubstitutionPass::addRand,
               &SubstitutionPass::addDoubleNeg};
    subFunc = {&SubstitutionPass::subNeg, &SubstitutionPass::subRand};
  }

  virtual bool runOnFunction(Function &F) {
    for (int i = 0; i < ObfTimes; i++) {
      for (auto &&bb : F) {
        for (auto &&inst : bb) {
          if (rng() % 100 >= ObfProbRate && !inst.isBinaryOp()) {
            continue;
          }
          // Instruction replacement is based on the type of opcode.
          switch (inst.getOpcode()) {
          case Instruction::Add:
            (this->*addFunc[rng() % addFunc.size()])(&inst);
            break;
          case Instruction::Sub:
            (this->*subFunc[rng() % subFunc.size()])(&inst);
            break;

          default:
            break;
          }
        }
      }
    }
    return true;
  }

  /// a = b + c
  /// a = b - (-c)
  void addNeg(Instruction *I) {
    IRBuilder<> builder(I);
    Value *c = builder.CreateNeg(I->getOperand(1));
    Value *a = builder.CreateSub(I->getOperand(0), c);
    I->replaceAllUsesWith(a);
  }

  /// r = rand(); a = b + r; a = a + c; a = a - r
  void addRand(Instruction *I) {
    IRBuilder<> builder(I);
    Constant *r = builder.getInt32(rng());
    Value *a = builder.CreateAdd(I->getOperand(0), r);
    a = builder.CreateAdd(a, I->getOperand(1));
    a = builder.CreateSub(a, r);
    I->replaceAllUsesWith(a);
  }

  /// a = -(-b + (-c))
  void addDoubleNeg(Instruction *I) {
    IRBuilder<> builder(I);
    Value *c = builder.CreateNeg(I->getOperand(1));
    Value *b = builder.CreateNeg(I->getOperand(0));
    Value *a = builder.CreateAdd(b, c);
    a = builder.CreateNeg(a);
    I->replaceAllUsesWith(a);
  }

  /// a = b + (-c)
  void subNeg(Instruction *I) {
    IRBuilder<> builder(I);
    Value *c = builder.CreateNeg(I->getOperand(1));
    Value *a = builder.CreateAdd(I->getOperand(0), c);
    I->replaceAllUsesWith(a);
  }

  /// r = rand(); a = b + r; a = a - c; a = a - r
  void subRand(Instruction *I) {
    IRBuilder<> builder(I);
    Constant *r = builder.getInt32(rng());
    Value *a = builder.CreateAdd(I->getOperand(0), r);
    a = builder.CreateSub(a, I->getOperand(1));
    a = builder.CreateSub(a, r);
    I->replaceAllUsesWith(a);
  }
};

char SubstitutionPass::ID = 2;
static RegisterPass<SubstitutionPass> X("subobf",
                                        "Enable Instruction Substitution");