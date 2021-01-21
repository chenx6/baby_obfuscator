#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/CommandLine.h"

#include <random>

using namespace llvm;

static cl::opt<int>
    ObfProbRate("bcf_prob",
                cl::desc("Choose the probability [%] each basic blocks will be "
                         "obfuscated by the -bcf pass"),
                cl::value_desc("probability rate"), cl::init(70), cl::Optional);

struct BongusFlowPass : public FunctionPass {
  static char ID;
  std::mt19937 rng;
  SmallVector<unsigned int, 13> integerOp;
  SmallVector<unsigned int, 5> floatOp;
  SmallVector<AllocaInst *, 0> allocaInsts;

  BongusFlowPass() : FunctionPass(ID), rng(std::random_device{}()) {
    integerOp = {Instruction::Add,  Instruction::Sub,  Instruction::Mul,
                 Instruction::UDiv, Instruction::SDiv, Instruction::URem,
                 Instruction::SRem, Instruction::Shl,  Instruction::LShr,
                 Instruction::AShr, Instruction::And,  Instruction::Or,
                 Instruction::Xor};
    floatOp = {Instruction::FAdd, Instruction::FSub, Instruction::FMul,
               Instruction::FDiv, Instruction::FRem};
  }

  virtual bool runOnFunction(Function &F) {
    // Put origin BB into vector.
    SmallVector<BasicBlock *, 0> targetBasicBlocks;
    for (BasicBlock &BB : F) {
      targetBasicBlocks.emplace_back(&BB);
    }
    // Put "alloca i32 ..." instruction into allocaInsts for further use
    findAllocInst(F.getEntryBlock());
    // Add bogus control flow to some BB.
    for (BasicBlock *BB : targetBasicBlocks) {
      if (rng() % 100 >= ObfProbRate) {
        continue;
      }
      BasicBlock *bogusBB = geneBogusFlow(BB, &F);
      addBogusFlow(BB, bogusBB, &F);
    }
    return true;
  }

  /// Put "alloca i32 ..." instruction into allocaInsts
  /// \return the size of allocaInsts
  int findAllocInst(BasicBlock &BB) {
    Type *Int32Ty = Type::getInt32Ty(BB.getContext());
    for (Instruction &inst : BB) {
      if (AllocaInst *allocaInst = dyn_cast<AllocaInst>(&inst)) {
        if (allocaInst->getAllocatedType() != Int32Ty) {
          continue;
        }
        allocaInsts.emplace_back(allocaInst);
      }
    }
    return allocaInsts.size();
  }

  /// Generate Bogus BasicBlock
  /// \param targetBB template BasicBlock
  /// \param F function
  BasicBlock *geneBogusFlow(BasicBlock *targetBB, Function *F) {
    ValueToValueMapTy VMap;
    const Twine &Name = "bogusBlock";
    BasicBlock *bogusBB = CloneBasicBlock(targetBB, VMap, Name, F);
    // Remap operands.
    BasicBlock::iterator ji = targetBB->begin();
    for (Instruction &i : *bogusBB) {
      // Loop over the operands of the instruction
      for (Use &opi : i.operands()) {
        // get the value for the operand
        Value *v = MapValue(opi, VMap, RF_None);
        if (v != nullptr) {
          opi = v;
        }
      }
      // Remap phi nodes' incoming blocks.
      if (PHINode *pn = dyn_cast<PHINode>(&i)) {
        for (unsigned int j = 0, e = pn->getNumIncomingValues(); j != e; ++j) {
          Value *v = MapValue(pn->getIncomingBlock(j), VMap, RF_None);
          if (v != nullptr) {
            pn->setIncomingBlock(j, cast<BasicBlock>(v));
          }
        }
      }
      // Remap attached metadata.
      SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
      i.getAllMetadata(MDs);
      // important for compiling with DWARF, using option -g.
      i.setDebugLoc(ji->getDebugLoc());
      ji++;
    }

    // Modify some instruction's Operands
    for (Instruction &i : *bogusBB) {
      if (!i.isBinaryOp()) {
        continue;
      }
      unsigned int opcode = i.getOpcode();
      unsigned int numOperands = i.getNumOperands();
      if (find(integerOp, opcode) != integerOp.end()) {
        i.setOperand(0, i.getOperand(rng() % numOperands));
      } else if (find(floatOp, opcode) != floatOp.end()) {
        i.setOperand(0, i.getOperand(rng() % numOperands));
      }
    }
    return bogusBB;
  }

  /// Put target BasicBlock and Bogus Block together
  void addBogusFlow(BasicBlock *targetBB, BasicBlock *bogusBB, Function *F) {
    // Split the block
    BasicBlock *targetBodyBB;
    if (targetBB->getFirstNonPHIOrDbgOrLifetime()) {
      targetBodyBB =
          targetBB->splitBasicBlock(targetBB->getFirstNonPHIOrDbgOrLifetime());
    } else {
      targetBodyBB = targetBB->splitBasicBlock(targetBB->begin());
    }

    // Modify the terminators to adjust the control flow.
    bogusBB->getTerminator()->eraseFromParent();
    targetBB->getTerminator()->eraseFromParent();

    // Add opaque predicate
    // if (x * (x + 1) % 2 == 0)
    IRBuilder<> bogusCondBuilder(targetBB);
    Value *loadInst;
    if (allocaInsts.size() != 0) {
      loadInst =
          bogusCondBuilder.CreateLoad(allocaInsts[rng() % allocaInsts.size()]);
    } else {
      GlobalVariable *globalValue = new GlobalVariable(
          *F->getParent(), bogusCondBuilder.getInt32Ty(), false,
          GlobalValue::PrivateLinkage, bogusCondBuilder.getInt32(rng()));
      loadInst = bogusCondBuilder.CreateLoad(globalValue);
    }
    Value *addInst =
        bogusCondBuilder.CreateAdd(loadInst, bogusCondBuilder.getInt32(1));
    Value *mulInst = bogusCondBuilder.CreateMul(loadInst, addInst);
    Value *remainInst =
        bogusCondBuilder.CreateSRem(mulInst, bogusCondBuilder.getInt32(2));
    Value *trueCond =
        bogusCondBuilder.CreateICmpEQ(remainInst, bogusCondBuilder.getInt32(0));
    bogusCondBuilder.CreateCondBr(trueCond, targetBodyBB, bogusBB);
    BranchInst::Create(targetBodyBB, bogusBB);

    // Split at this point (we only want the terminator in the second part)
    BasicBlock *targetBodyEndBB =
        targetBodyBB->splitBasicBlock(--targetBodyBB->end());
    // erase the terminator created when splitting.
    targetBodyBB->getTerminator()->eraseFromParent();
    // We add at the end a new always true condition
    IRBuilder<> endCondBuilder(targetBodyBB, targetBodyBB->end());
    Value *endCond =
        endCondBuilder.CreateICmpEQ(remainInst, bogusCondBuilder.getInt32(0));
    endCondBuilder.CreateCondBr(endCond, targetBodyEndBB, bogusBB);
  }
};

char BongusFlowPass::ID = 1;
static RegisterPass<BongusFlowPass> X("boguscf",
                                      "inserting bogus control flow");