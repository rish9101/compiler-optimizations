#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallVector.h>
#include "Dataflow/dataflow.h"

namespace llvm
{

    class DominatorsDFA : public Dataflow<BasicBlock *>
    {
    public:
        DominatorsDFA(Function &F) : Dataflow(FORWARDS)
        {
            for (auto &BB : F)
            {
                OutBB[&BB] = BitVector(256, true);
                InBB[&BB] = BitVector(256, false);
            }
            // Set entry block's OutBB to correct value
            auto &entryBB = F.getEntryBlock();
            OutBB[&entryBB].reset();
            getDomain().push_back(&entryBB);
            OutBB[&entryBB].set(std::find(getDomain().begin(), getDomain().end(), &entryBB) - getDomain().begin());
        }

        std::vector<BasicBlock *> getDomList(BasicBlock *BB)
        {
            std::vector<BasicBlock *> doms;
            for (auto bit : OutBB[BB].set_bits())
            {
                doms.push_back(getDomain()[bit]);
            }
            return doms;
        }

        void printSet(BitVector bv)
        {

            for (auto set_bit : bv.set_bits())
            {
                outs() << getDomain()[set_bit]->getName() << ", ";
            }
            outs() << "\n";
        }

        BitVector meetOp(std::vector<BitVector> meetCandidates) override
        {
            if (meetCandidates.size() == 0)
                return BitVector(256, false);
            BitVector newIn = BitVector(256, true);
            for (auto bv : meetCandidates)
            {
                newIn &= bv;
            }
            return newIn;
        }

        BitVector transferFunc(BasicBlock &BB) override
        {
            auto newOut = BitVector(256, false);
            newOut |= InBB[&BB];
            BitVector genSet, killSet;

            // KillSet will be empty, but we use the method to maintain
            // coherence with  the framework
            std::tie(genSet, killSet) = getGenAndKillSet(BB);

            newOut |= genSet;
            return newOut;
        }

        std::pair<BitVector, BitVector> getGenAndKillSet(BasicBlock &BB) override
        {
            auto killSet = BitVector(256, false);
            auto genSet = BitVector(256, false);

            if (std::find(getDomain().begin(), getDomain().end(), &BB) == getDomain().end())
            {
                getDomain().push_back(&BB);
            }
            int bitIdx = std::find(getDomain().begin(), getDomain().end(), &BB) - getDomain().begin();
            genSet.set(bitIdx);

            return std::pair<BitVector, BitVector>(genSet, killSet);
        }
    };

    class DominatorsPass : public FunctionPass
    {
    public:
        std::map<BasicBlock *, SmallVector<BasicBlock *, 64>> dominators;

        static char ID;

        DominatorsPass() : FunctionPass(ID) {}

        virtual bool runOnFunction(Function &F)
        {

            // ---------------------
            // INITIALIZATION STEP FOR DATAFLOW
            // ---------------------

            DominatorsDFA *domfa = new DominatorsDFA(F);

            domfa->performDFA(F);

            for (auto &BB : F)
            {
                auto bv = domfa->InBB[&BB];
                for (auto bit : bv.set_bits())
                    dominators[&BB].push_back(domfa->getDomain()[bit]);
            }

            for (auto &BB : F)
            {
                outs() << "Printing Dominators for " << BB.getName() << "\n";
                domfa->printSet(domfa->InBB[&BB]);
            }

            return false;
        }

        virtual void
        getAnalysisUsage(AnalysisUsage &AU) const
        {
            AU.setPreservesAll();
        }

        std::map<BasicBlock *, SmallVector<BasicBlock *, 64>> getDominators()
        {
            return dominators;
        }

    private:
    };

    char DominatorsPass::ID = 0;
    RegisterPass<DominatorsPass> X("dominators",
                                   "ECE/CS 5544 Dominators Pass");
};