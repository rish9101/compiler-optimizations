#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include <llvm/Pass.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/PostOrderIterator.h>
#include "Dataflow/dataflow.h"

// using namespace llvm;
namespace llvm
{
    std::string getShortValueName(Value *v)
    {
        if (v->getName().str().length() > 0)
        {
            return "%" + v->getName().str();
        }
        else if (isa<Instruction>(v))
        {
            std::string s = "";
            raw_string_ostream *strm = new raw_string_ostream(s);
            v->print(*strm);
            std::string inst = strm->str();
            size_t idx1 = inst.find("%");
            size_t idx2 = inst.find(" ", idx1);
            if (idx1 != std::string::npos && idx2 != std::string::npos)
            {
                return inst.substr(idx1, idx2 - idx1);
            }
            else
            {
                return "\"" + inst + "\"";
            }
        }
        else if (ConstantInt *cint = dyn_cast<ConstantInt>(v))
        {
            std::string s = "";
            raw_string_ostream *strm = new raw_string_ostream(s);
            cint->getValue().print(*strm, true);
            return strm->str();
        }
        else
        {
            std::string s = "";
            raw_string_ostream *strm = new raw_string_ostream(s);
            v->print(*strm);
            std::string inst = strm->str();
            return "\"" + inst + "\"";
        }
    }

    bool isLive(Instruction &I)
    {

        return I.isTerminator() ||
               isa<DbgInfoIntrinsic>(&I) || isa<LandingPadInst>(&I) || I.mayHaveSideEffects();
    }

    bool isAssignmentInstruction(Instruction &I)
    {

        // We cannot certainly say that an instruction is an assignment instruction
        // e.g add a1, a2 CAN be used to assign a value to a variable or not.
        // function calls are also of similar nature.
        // However, if return type of an inst is void, it is sure to NOT return a value
        // Store Instructions are technically assignment instructions only

        // TODO: Deal with function calls.
        return !(I.getType()->isVoidTy());
    }

    /**
     * @brief The Dataflow analysis class for faint analysis. This class defines all the operators and attributes for DFA
     *
     */
    class DCEDFA
    {
    public:
        std::vector<Value *> faintVariables;

        ValueMap<Instruction *, BitVector> FaintGenS;
        ValueMap<Instruction *, BitVector> FaintKillS;
        ValueMap<Instruction *, BitVector> FaintInS;
        ValueMap<Instruction *, BitVector> FaintOutS;

        DCEDFA(Function &F)
        {
            for (auto &BB : F)
            {
                for (auto &I : BB)
                {
                    FaintGenS[&I] = BitVector(256, false);
                    FaintInS[&I] = BitVector(256, false);
                    FaintOutS[&I] = BitVector(256, false);
                    FaintKillS[&I] = BitVector(256, false);
                }
            }
        }

        void printSet(BitVector bv)
        {

            outs() << "[   ";
            for (auto bit : bv.set_bits())
            {
                if (bit < faintVariables.size())
                    outs() << getShortValueName(faintVariables[bit]) << ",";
            }
            outs() << "  ]    ";
        }

        std::pair<BitVector, BitVector> getGenAndKillSet(Instruction &I)
        {
            // Cannot cache Kill set since they are dependent

            // In SSA IR, genSet will be equal to lhs for faint analysis
            // since a variable cannot appear on both lhs and rhs of a statement

            BitVector UseS(256, false);
            BitVector Rhs(256, false);

            FaintKillS[&I].reset();

            if (isAssignmentInstruction(I) == true)
            {

                // Calculating GenSetS (same as Lhs in SSA)
                if (std::find(faintVariables.begin(), faintVariables.end(), dyn_cast<Value>(&I)) == faintVariables.end())
                {
                    faintVariables.push_back(&I);
                }
                FaintGenS[&I].set(std::find(faintVariables.begin(), faintVariables.end(), dyn_cast<Value>(&I)) - faintVariables.begin());

                // Calculating DepKillS
                if (FaintOutS[&I][std::find(faintVariables.begin(), faintVariables.end(), dyn_cast<Value>(&I)) - faintVariables.begin()] == 0)
                {
                    for (auto &operandUse : I.operands())
                    {
                        auto operand = operandUse.get();

                        if (isa<Constant>(operand))
                        {
                            continue;
                        }
                        if (std::find(faintVariables.begin(), faintVariables.end(), operand) == faintVariables.end())
                        {
                            faintVariables.push_back(operand);
                        }
                        FaintKillS[&I].set(std::find(faintVariables.begin(), faintVariables.end(), operand) - faintVariables.begin());
                    }
                }
            }

            else
            {
                // Calculating UseS
                // TODO: Deal with function calls for UseS
                if (isa<CallInst>(&I))
                {
                    auto callInst = dyn_cast<CallInst>(&I);
                    for (auto &argUse : callInst->args())
                    {
                        auto arg = argUse.get();
                        if (std::find(faintVariables.begin(), faintVariables.end(), arg) == faintVariables.end())
                        {
                            faintVariables.push_back(arg);
                        }
                        UseS.set(std::find(faintVariables.begin(), faintVariables.end(), arg) - faintVariables.begin());
                    }
                }
                else if (isa<BranchInst>(&I))
                {
                    auto branchInst = dyn_cast<BranchInst>(&I);
                    if (branchInst->isConditional() == true)
                    {
                        auto condVar = branchInst->getCondition();
                        if (std::find(faintVariables.begin(), faintVariables.end(), condVar) == faintVariables.end())
                        {
                            faintVariables.push_back(condVar);
                        }
                        UseS.set(std::find(faintVariables.begin(), faintVariables.end(), condVar) - faintVariables.begin());
                    }
                }
                else
                {
                    for (size_t i = 0; i < I.getNumOperands(); ++i)
                    {

                        auto operand = I.getOperand(i);
                        if (isa<Constant>(operand))
                            continue;
                        if (std::find(faintVariables.begin(), faintVariables.end(), operand) == faintVariables.end())
                        {
                            faintVariables.push_back(operand);
                        }
                        UseS.set(std::find(faintVariables.begin(), faintVariables.end(), operand) - faintVariables.begin());
                    }
                }
            }

            FaintKillS[&I] |= UseS;
            return std::pair<BitVector, BitVector>(FaintGenS[&I], FaintKillS[&I]);
        }

        BitVector transferFunc(Instruction &I)
        {

            BitVector newFaintInS(256, false);

            BitVector FaintGenS(256, false), FaintKillS(256, false);
            std::tie<BitVector, BitVector>(FaintGenS, FaintKillS) = getGenAndKillSet(I);

            newFaintInS |= FaintOutS[&I];
            FaintKillS.flip();
            newFaintInS &= FaintKillS;

            newFaintInS |= FaintGenS;
            return newFaintInS;
        }

        // Meet Operator to be applies on successors of an Instruction since this DFA is non seperable
        // Basicblocks however, can only be exited on a branching instruction (or terminators).
        // To get successors of such branching instructions, we'll therefore actually use the first instruction
        // of the successor basic blocks of these blocks.
        BitVector meetOp(Instruction &I)
        {
            BitVector newOut(256, true);
            if (isa<ReturnInst>(&I))
            {
                // Out[Exit node] = T
                return newOut;
            }
            if (isa<BranchInst>(&I))
            {

                auto BB = I.getParent();
                for (auto successorBB : successors(BB))
                {
                    auto &successorInst = successorBB->front();
                    newOut &= FaintInS[&successorInst];
                }
                return newOut;
            }

            auto successorInst = I.getNextNode();
            newOut &= FaintInS[successorInst];
            return newOut;
        }

        void performDFA(Function &F)
        {
            bool changed = true;

            auto &bbList = F.getBasicBlockList();

            std::vector<Instruction *> insts;

            for (auto bb = bbList.rbegin(); bb != bbList.rend(); ++bb)
            {
                for (auto inst = (*bb).rbegin(); inst != (*bb).rend(); ++inst)
                {
                    insts.push_back(&(*inst));
                }
            }

            // BACKWARDS PASS

            while (changed == true)
            {
                changed = false;
                for (auto inst : insts)
                {

                    auto oldOut = FaintOutS[inst];
                    auto oldIn = FaintInS[inst];

                    auto newOut = meetOp(*inst);

                    FaintOutS[inst].reset();
                    FaintOutS[inst] |= newOut;

                    auto newIn = transferFunc(*inst);
                    FaintInS[inst].reset();
                    FaintInS[inst] |= newIn;

                    if ((oldOut != FaintOutS[inst]) || (oldIn != FaintInS[inst]))
                        changed = true;
                }
            }

            for (auto inst : insts)
            {
                outs() << *inst << " ::: ";
                printSet(FaintOutS[inst]);
                printSet(FaintInS[inst]);
                outs() << "\n\n";
            }
        }

        bool canBeRemoved(Instruction &I)
        {

            int bit = std::find(faintVariables.begin(), faintVariables.end(), &I) - faintVariables.begin();
            return (FaintOutS[&I][bit] == 1);
        }
    };

    class DCEPass : public FunctionPass
    {
    public:
        static char ID;

        virtual bool runOnFunction(Function &F) override
        {
            // auto dcedfa = new DCEDFA();

            auto dcedfa = new DCEDFA(F);

            dcedfa->performDFA(F);

            auto &bbList = F.getBasicBlockList();

            std::vector<Instruction *> insts;

            for (auto bb = bbList.rbegin(); bb != bbList.rend(); ++bb)
            {
                for (auto inst = (*bb).rbegin(); inst != (*bb).rend(); ++inst)
                {
                    insts.push_back(&(*inst));
                }
            }

            for (auto I : insts)
            {

                if (isLive(*I) || !(isAssignmentInstruction(*I)))
                    continue;
                // If the result of an assignment instruction is in FaintOut of the instruction
                // then we can remove it as the assigned variable isn't used further
                bool canRemove = dcedfa->canBeRemoved(*I);
                outs() << *I << (canRemove ? " Can" : " Can't")
                       << " Be Removed\n";
                if (canRemove)
                    I->eraseFromParent();
            }

            return false;
        }

        virtual void
        getAnalysisUsage(AnalysisUsage &AU) const override
        {
            AU.setPreservesAll();
        }

        DCEPass() : FunctionPass(ID){};
    };

    char DCEPass::ID = 0;

    // void addDCEPass(const PassManagerBuilder & /* unused */,
    //                 legacy::PassManagerBase &PM)
    // {
    //     PM.add(new DCEPass());
    // }

    // Make the pass known to opt.
    RegisterPass<DCEPass> X("dead-code", "Dead Code Elimination Pass");
    // Tell frontends to run the pass automatically.
    // static struct RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
    //                                        addDCEPass);
};