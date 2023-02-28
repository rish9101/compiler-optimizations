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
    LivenessDataFlow(Function &F) : Dataflow(BACKWARDS)
    {

      for (auto &BB : F)
      {
        // For Liveness analysis In[EXIT] = NULL
        // So initializing it
        InBB[&BB] = BitVector(64, false);
        OutBB[&BB] = BitVector(64, false);
      }
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
