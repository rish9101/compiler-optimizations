#include <utility>
#include <map>

#include "Common.h"

/**
 * This Point To analysis is based on the DFA analysis of the
 * same taken from KSK Chapter - 4
 *
 * Phase 1 - Ignoring pointer indirection (i.e only considering level one pointers)
 * we implement a basic pointer analysis i.e pointers do not go beyond int * a
 *
 *
 *
 */

struct Memory
{
    int i;
};

bool operator<(const Memory &lhs, const Memory &rhs)
{
    return lhs.i < rhs.i;
}

class PointsToDFA
{
    SmallVector<std::pair<Value *, Memory>, 64> domain;

    std::map<Instruction *, BitVector> MayInS;
    std::map<Instruction *, BitVector> MayOutS;
    std::map<Instruction *, BitVector> MustInS;
    std::map<Instruction *, BitVector> MustOutS;
};

class PointsToPass : public FunctionPass
{
private:
    std::vector<Instruction *> mallocInstructions;

public:
    static char ID;

    PointsToPass() : FunctionPass(ID){};

    virtual bool runOnFunction(Function &F);
    virtual bool runOnModule(Module &M);
};

char PointsToPass::ID = 0;

bool PointsToPass::runOnFunction(Function &F)
{

    std::vector<Instruction *> allInsts;

    for (auto &BB : F)
    {

        for (auto &I : BB)
        {

            if (auto callInst = dyn_cast<CallInst>(&I))
            {
                if (callInst->getCalledFunction()->getName() == "malloc")
                {
                    outs() << formatv("Found a call to malloc {0} - dest ptr is {1}\n", *callInst, getShortValueName(callInst));
                    mallocInstructions.push_back(callInst);
                }
            }
            allInsts.push_back(&I);
        }
    }
    return false;
}

bool PointsToPass::runOnModule(Module &M)
{
    return false;
}

void addPointsToPass(const PassManagerBuilder & /* unused */,
                     legacy::PassManagerBase &PM)
{
    PM.add(new PointsToPass());
}

// Register the pass so `opt -mempass` runs it.
static RegisterPass<PointsToPass> A("heap-liveness", "Perform Points To analysis");
// Tell frontends to run the pass automatically.
static RegisterStandardPasses B(PassManagerBuilder::EP_VectorizerStart,
                                addPointsToPass);
static RegisterStandardPasses
    C(PassManagerBuilder::EP_EnabledOnOptLevel0, addPointsToPass);