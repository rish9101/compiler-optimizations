// ECE/CS 5544 S22 Assignment 2: reaching.cpp
// Group:

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace
{

  // In order to implement the bitvector here, we'll use Value *
  class ReachingDataFlow : public Dataflow<Value *>
  {
  public:
    ReachingDataFlow(Function &F) : Dataflow(FORWARDS)
    {
      int i = 0;
      for (auto &BB : F)
      {

        InBB[&BB] = BitVector(256, false);

        OutBB[&BB] = BitVector(256, false);
        ++i;
      }
    }

    void printBinOps(BitVector bv)
    {

      outs() << "{";
      for (auto setIdx : bv.set_bits())
      {
        outs() << "\n\td" << setIdx << ": ";
        getDomain()[setIdx]->print(outs());
        outs() << "";
      }
      outs() << " }\n";
    }

    BitVector meetOp(BasicBlock &BB, bool isEntry = false) override
    {
      auto predecessorBBs = predecessors(&BB);
      auto newIn = BitVector(256, false);
      // Calculating IN[BB] = &(Out[P]) for every predecessor P of BB
      for (auto predBB : predecessorBBs)
      {
        newIn |= OutBB[predBB];
      }
      return newIn;
    }

    BitVector transferFunc(BasicBlock &BB) override
    {
      BitVector genSet, killSet;
      std::tie(genSet, killSet) = getGenAndKillSet(BB);

      auto newOut = killSet.flip();
      newOut &= InBB[&BB];
      newOut |= genSet;
      return newOut;
    };

    std::pair<BitVector, BitVector> getGenAndKillSet(BasicBlock &BB) override
    {
      // Gen And Kill sets need to be computed together

      if ((GenBB.find(&BB) != GenBB.end()) && (KillBB.find(&BB) != KillBB.end()))
      {
        return std::pair<BitVector, BitVector>(GenBB[&BB], KillBB[&BB]);
      }

      BitVector genSet(256, false);
      BitVector killSet(256, false);

      for (auto &inst : BB)
      {
        // d1: a = 4
        // d2: a = 6
        // genSet = {d2}, killSet = {d1, d2}
        //

        auto destName = getShortValueName(&inst);
        if (!isa<BinaryOperator>(inst) && inst.getNumUses() == 0)
        {
          // Instruction probably not of the form
          // a = b op c
          continue;
        }
        getDomain().push_back(&inst);
        auto defSeen = getDomain();

        auto genIdx = std::find(getDomain().begin(), getDomain().end(), &inst) - getDomain().begin();

        genSet.set(genIdx);

        for (auto itr = getDomain().begin(); itr != std::find(getDomain().begin(), getDomain().end(), &inst); ++itr)
        {
          if (getShortValueName(*itr) == destName)
          {
            genSet.reset(itr - getDomain().begin());
          }
        }
      }

      /**
       * In SSA format, a variable is defined only once, and thus it is never redefined
       * So, the killset of any Basic Block is always empty in SSA
       */

      // Cache the GenSet and KillSet for every Block to use for next iteration
      GenBB.insert(std::pair<BasicBlock *, BitVector>(&BB, genSet));
      KillBB.insert(std::pair<BasicBlock *, BitVector>(&BB, killSet));

      return std::pair<BitVector, BitVector>(genSet, killSet);
    }
  };

  class Reaching : public FunctionPass
  {
  public:
    static char ID;

    Reaching() : FunctionPass(ID) {}

    bool doInitialization(Module &M)
    {
      outs() << "Reaching Def Pass\n";
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

      ReachingDataFlow *rdf = new ReachingDataFlow(F);

      rdf->performDFA(F);

      for (auto &BB : F)
      {
        BitVector genSet, killSet;
        std::tie(genSet, killSet) = rdf->getGenAndKillSet(BB);

        outs() << "BB Name - " << getShortValueName(&BB) << "\n";
        outs() << "Gen BB - ";
        rdf->printBinOps(genSet);
        outs() << "Kill BB - ";
        rdf->printBinOps(killSet);
        outs() << "IN[BB] - ";
        rdf->printBinOps(rdf->InBB[&BB]);
        outs() << "OUT[BB] - ";
        rdf->printBinOps(rdf->OutBB[&BB]);
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

  char Reaching::ID = 0;
  RegisterPass<Reaching> X("reaching", "ECE/CS 5544 Reaching");
}
