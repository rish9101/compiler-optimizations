// ECE/CS 5544 S22 Assignment 2: liveness.cpp
// Group:

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace
{

  class LivenessDataFlow : public Dataflow<Value *>
  {
  public:
    LivenessDataFlow(Function &F) : Dataflow(BACKWARDS)
    {

      for (auto &BB : F)
      {
        // For Liveness analysis In[EXIT] = NULL
        // So initializing it
        InBB[&BB] = BitVector(256, false);
        OutBB[&BB] = BitVector(256, false);
      }
    }

    void printVariableBV(BitVector bv)
    {

      outs() << "{ ";
      for (auto setIdx : bv.set_bits())
      {
        outs() << getShortValueName(getDomain()[setIdx]) << "  ";
      }
      outs() << " }\n";
    }

    BitVector meetOp(BasicBlock &BB, bool isEntry = false) override
    {
      auto succBBs = successors(&BB);

      BitVector newOut = BitVector(256, false);
      for (auto succBB : succBBs)
      {
        newOut |= InBB[succBB];
      }
      return newOut;
    }

    BitVector transferFunc(BasicBlock &BB) override
    {
      BitVector defSet, useSet;
      std::tie(defSet, useSet) = getGenAndKillSet(BB);

      auto newIn = defSet.flip();
      newIn &= OutBB[&BB];
      newIn |= useSet;
      return newIn;
    }

    std::pair<BitVector, BitVector> getGenAndKillSet(BasicBlock &BB) override
    {

      // Check if we've already computed and cached the Gen and Kill sets
      if ((GenBB.find(&BB) != GenBB.end()) && (KillBB.find(&BB) != KillBB.end()))
      {
        return std::pair<BitVector, BitVector>(GenBB[&BB], KillBB[&BB]);
      }

      BitVector defSet(256, false);
      BitVector useSet(256, false);
      for (auto &inst : BB)
      {

        // INSERT VALUE * INTO USE SET
        for (auto i = 0; i < inst.getNumOperands(); ++i)
        {
          auto op = inst.getOperand(i);
          if (isa<ConstantData>(op) || isa<BasicBlock>(op))
          {
            continue;
          }
          if (std::find(getDomain().begin(), getDomain().end(), op) == getDomain().end())
          {
            getDomain().push_back(op);
          }
          useSet.set(std::find(getDomain().begin(), getDomain().end(), op) - getDomain().begin());
        }

        if (!isa<BinaryOperator>(inst) && inst.getNumUses() == 0)
        {
          // Not an assignment operation
          // Instructions like br, ret etc
          continue;
        }

        // INSERT VALUE * INTO DEF SET

        if (std::find(getDomain().begin(), getDomain().end(), &inst) == getDomain().end())
        {
          Value *v = (Value *)(&inst);
          getDomain().push_back(v);
        }
        int defIdx = std::find(getDomain().begin(), getDomain().end(), &inst) - getDomain().begin();
        defSet.set(defIdx);
      }

      // Cache the GenSet and KillSet for every Block to use for next iteration
      GenBB.insert(std::pair<BasicBlock *, BitVector>(&BB, defSet));
      KillBB.insert(std::pair<BasicBlock *, BitVector>(&BB, useSet));

      return std::pair<BitVector, BitVector>(defSet, useSet);
    }
  };

  class Liveness : public FunctionPass
  {
  public:
    static char ID;

    Liveness() : FunctionPass(ID) {}

    bool doInitialization(Module &M)
    {

      outs() << "Liveness Analysis Pass\n";
      return false;
    }

    virtual bool runOnFunction(Function &F)
    {

      for (auto &BB : F)
      {
        outs() << BB;
      }

      // ---------------------
      // INITIALIZATION STEP FOR DATAFLOW
      // ---------------------

      LivenessDataFlow *ldf = new LivenessDataFlow(F);

      ldf->performDFA(F);

      for (auto &BB : F)
      {
        BitVector defSet, useSet;
        std::tie(defSet, useSet) = ldf->getGenAndKillSet(BB);

        outs() << "BB Name - " << BB.getName() << "\n";
        outs() << "Def BB - ";
        ldf->printVariableBV(defSet);
        outs() << "Use BB - ";
        ldf->printVariableBV(useSet);
        outs() << "IN[BB] - ";
        ldf->printVariableBV(ldf->InBB[&BB]);
        outs() << "OUT[BB] - ";
        ldf->printVariableBV(ldf->OutBB[&BB]);
        outs() << "-------------------\n\n";
      }

      // Did not modify the incoming Function.
      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
      AU.setPreservesAll();
    }

  private:
  };

  char Liveness::ID = 0;
  RegisterPass<Liveness> X("liveness", "ECE/CS 5544 Liveness");
}
