#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Scalar.h"

#include <random>

using namespace llvm;

struct FlatteningPass : public FunctionPass {
  static char ID;
  std::mt19937 rng;

  FlatteningPass() : FunctionPass(ID), rng(std::random_device{}()) {}

  bool runOnFunction(Function &F) override {
    // Only one BB in this Function
    if (F.size() <= 1) {
      return false;
    }

    // Insert All BB into originBB
    SmallVector<BasicBlock *, 0> originBB;
    for (BasicBlock &bb : F) {
      originBB.emplace_back(&bb);
      if (isa<InvokeInst>(bb.getTerminator())) {
        return false;
      }
    }

    // Remove first BB
    originBB.erase(originBB.begin());

    // If firstBB's terminator is BranchInst, then split into two blocks
    BasicBlock *firstBB = &*F.begin();
    Instruction *firstBBTerminator = firstBB->getTerminator();
    if (isa<BranchInst>(firstBBTerminator) ||
        isa<IndirectBrInst>(firstBBTerminator)) {
      BasicBlock::iterator iter = firstBB->end();
      if (firstBB->size() > 1) {
        --iter;
      }
      BasicBlock *tempBB = firstBB->splitBasicBlock(--iter);
      originBB.insert(originBB.begin(), tempBB);
    }

    // Remove firstBB
    firstBB->getTerminator()->eraseFromParent();

    // Create main loop
    BasicBlock *loopEntry = BasicBlock::Create(F.getContext(), "Entry", &F);
    BasicBlock *loopEnd = BasicBlock::Create(F.getContext(), "End", &F);
    BasicBlock *swDefault = BasicBlock::Create(F.getContext(), "Default", &F);
    // Create switch variable
    IRBuilder<> entryBuilder(firstBB, firstBB->end());
    AllocaInst *swPtr = entryBuilder.CreateAlloca(entryBuilder.getInt32Ty());
    StoreInst *storeRng =
        entryBuilder.CreateStore(entryBuilder.getInt32(rng()), swPtr);
    entryBuilder.CreateBr(loopEntry);
    // Create switch statement
    IRBuilder<> swBuilder(loopEntry);
    LoadInst *swVar = swBuilder.CreateLoad(swPtr);
    SwitchInst *swInst = swBuilder.CreateSwitch(swVar, swDefault, 0);
    BranchInst *dfTerminator = BranchInst::Create(loopEntry, swDefault);
    BranchInst *toLoopEnd = BranchInst::Create(loopEntry, loopEnd);

    // Put all BB into switch Instruction
    for (BasicBlock *bb : originBB) {
      bb->moveBefore(loopEnd);
      swInst->addCase(swBuilder.getInt32(rng()), bb);
    }

    // Recalculate switch Instruction
    for (BasicBlock *bb : originBB) {
      switch (bb->getTerminator()->getNumSuccessors()) {
      case 0:
        // No terminator
        break;
      case 1: {
        // Terminator is a non-condition jump
        Instruction *terminator = bb->getTerminator();
        BasicBlock *sucessor = terminator->getSuccessor(0);
        // Find sucessor's case condition
        ConstantInt *caseNum = swInst->findCaseDest(sucessor);
        if (caseNum == nullptr) {
          caseNum = swBuilder.getInt32(rng());
        }
        // Connect this BB to sucessor
        IRBuilder<> caseBuilder(bb, bb->end());
        caseBuilder.CreateStore(caseNum, swPtr);
        caseBuilder.CreateBr(loopEnd);
        terminator->eraseFromParent();
      } break;
      case 2: {
        // Terminator is a condition jump
        Instruction *terminator = bb->getTerminator();
        ConstantInt *trueCaseNum =
            swInst->findCaseDest(terminator->getSuccessor(0));
        ConstantInt *falseCaseNum =
            swInst->findCaseDest(terminator->getSuccessor(1));
        if (trueCaseNum == nullptr) {
          trueCaseNum = swBuilder.getInt32(rng());
        }
        if (falseCaseNum == nullptr) {
          falseCaseNum = swBuilder.getInt32(rng());
        }
        IRBuilder<> caseBuilder(bb, bb->end());
        if (BranchInst *endBr = dyn_cast<BranchInst>(bb->getTerminator())) {
          // Select the next BB to be executed
          Value *selectInst = caseBuilder.CreateSelect(
              endBr->getCondition(), trueCaseNum, falseCaseNum);
          caseBuilder.CreateStore(selectInst, swPtr);
          caseBuilder.CreateBr(loopEnd);
          terminator->eraseFromParent();
        }
      } break;
      }
    }

    // Set swVar's origin value, let the first BB executed first
    ConstantInt *caseCond = swInst->findCaseDest(*originBB.begin());
    storeRng->setOperand(0, caseCond);

    // Demote register and phi to memory
    FunctionPass *reg2memPass = createDemoteRegisterToMemoryPass();
    reg2memPass->runOnFunction(F);
    return true;
  }
};

char FlatteningPass::ID = 3;
static RegisterPass<FlatteningPass> X("flattening", "Call graph flattening");