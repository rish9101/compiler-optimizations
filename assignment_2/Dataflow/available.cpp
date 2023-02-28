// ECE/CS 5544 S22 Assignment 2: available.cpp
// Group:

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallVector.h"

#include "dataflow.h"
#include "available-support.h"

#include <vector>
#include <utility>
#include <algorithm>

using namespace llvm;
using namespace std;

namespace
{

  class AvailableExpressionDataflow : public Dataflow<Expression>
  {

  public:
    void printBV(BitVector bv)
    {

      for (int i = bv.size() - 1; i >= 0; --i)
      {
        outs() << bv[i] << ", ";
      }
      outs() << "\n";
    }

    AvailableExpressionDataflow(Function &F) : Dataflow(FORWARDS)
    {
      int i = 0;
      for (auto &BB : F)
      {

        if (i == 0)
        {
          // This is the first landing block
          InBB[&BB] = BitVector(64, false);
        }
        else
        {
          InBB[&BB] = BitVector(64, true);
        }

        OutBB[&BB] = BitVector(64, true);

        outs() << "Basic Block number " << i << " " << &BB << BB << "\n";
        ++i;
      }
    };

    /**
     * @brief This is the transfer function, which calculates the
     * IN[B] or OUT[B] based on the data flow INSIDE a basic block
     *
     */
    BitVector transferFunc(BasicBlock &BB) override
    {
      BitVector genSet, killSet;
      tie(genSet, killSet) = getGenAndKillSet(BB);

      auto newOut = killSet.flip();
      newOut &= InBB[&BB];
      newOut |= genSet;
      return newOut;
    };

    std::vector<Expression> getExprFromBitVector(BitVector bv)
    {

      std::vector<Expression> exprSet;
      for (auto setBit : bv.set_bits())
      {
        exprSet.push_back(getDomain()[setBit]);
      }
      return exprSet;
    }

    BitVector meetOp(pred_range predecessorBBs, bool isEntryBB) override
    {
      auto newIn = BitVector(64, true);
      if (isEntryBB == true)
      {
        // This is the first landing block. It has no predecessors. So IN will never be updated
        newIn.flip();
      }
      // Calculating IN[BB] = &(Out[P]) for every predecessor P of BB
      for (auto predBB : predecessorBBs)
      {
        if (isEntryBB == true)
        {
          // Even the landing block has a predecessor. We thus flip the bits again
          newIn.flip();
        }
        newIn &= OutBB[predBB];
      }
      return newIn;
    };

    std::pair<BitVector, BitVector> getGenAndKillSet(BasicBlock &BB) override
    {
      // Gen And Kill sets need to be computed together

      if ((GenBB.find(&BB) != GenBB.end()) && (KillBB.find(&BB) != KillBB.end()))
      {
        return std::pair<BitVector, BitVector>(GenBB[&BB], KillBB[&BB]);
      }

      BitVector genSet(64, false);
      BitVector killSet(64, false);

      for (auto &inst : BB)
      {
        BinaryOperator *binInst = dyn_cast<BinaryOperator>(&inst);
        if (binInst != nullptr)
        {
          // We have a binary operator
          auto expr = std::find(getDomain().begin(), getDomain().end(), Expression(binInst));

          if (expr == getDomain().end())
          {
            getDomain().push_back(Expression(binInst));
          }
          int exprIdx = std::find(getDomain().begin(), getDomain().end(), Expression(binInst)) - getDomain().begin();

          // Expression added to GenSet

          genSet.set(exprIdx);

          // Remove expressions from genSet too
          std::string destName = getShortValueName(binInst);
          BitVector bv(64, false); // bv represents all the expressions killed by this instruction
          for (auto ex : getDomain())
          {
            if ((getShortValueName(ex.v1) == destName) || (getShortValueName(ex.v2) == destName))
            {
              bv.set(std::find(getDomain().begin(), getDomain().end(), ex) - getDomain().begin());
            }
          }
          // KillSet is just OR of killset for every statement in the BB

          killSet |= bv;

          // Let's get the genset
          genSet &= (bv.flip());
        }
      }

      // Cache the GenSet and KillSet for every Block to use for next iteration
      GenBB.insert(std::pair<BasicBlock *, BitVector>(&BB, genSet));
      KillBB.insert(std::pair<BasicBlock *, BitVector>(&BB, killSet));

      return std::pair<BitVector, BitVector>(genSet, killSet);
    }
  };

  class AvailableExpressionsPass : public FunctionPass
  {

  public:
    static char ID;

    AvailableExpressionsPass() : FunctionPass(ID) {}

    void printBV(BitVector bv)
    {

      for (int i = bv.size() - 1; i >= 0; --i)
      {
        outs() << bv[i] << ", ";
      }
      outs() << "\n";
    }

    virtual bool runOnFunction(Function &F)
    {

      // ---------------------
      // INITIALIZATION STEP FOR DATAFLOW
      // ---------------------

      AvailableExpressionDataflow *avf = new AvailableExpressionDataflow(F);

      avf->performDFA(F);

      for (auto &BB : F)
      {
        BitVector genSet, killSet;
        std::tie(genSet, killSet) = avf->getGenAndKillSet(BB);

        outs() << "BB Name - " << BB.getName() << "\n";
        outs() << "Gen BB - ";
        printSet(avf->getExprFromBitVector(genSet));
        outs() << "Kill BB - ";
        printSet(avf->getExprFromBitVector(killSet));
        outs() << "IN[BB] - ";
        printSet(avf->getExprFromBitVector(avf->InBB[&BB]));
        outs() << "OUT[BB] - ";
        printSet(avf->getExprFromBitVector(avf->OutBB[&BB]));
        outs() << "-------------------\n\n";
      }

      return false;
    }

    virtual void
    getAnalysisUsage(AnalysisUsage &AU) const
    {
      AU.setPreservesAll();
    }

  private:
  };

  char AvailableExpressionsPass::ID = 0;
  RegisterPass<AvailableExpressionsPass> X("available",
                                           "ECE/CS 5544 Available Expressions");
}
