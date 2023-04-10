#include <llvm/Analysis/LoopPass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/IR/IRBuilder.h>
#include "Dataflow/dataflow.h"

#include "Dominators/src/Pass.cc"

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

    class LICMPass : public LoopPass
    {
    public:
        static char ID;
        LICMPass() : LoopPass(ID){};
        SmallVector<Value *, 64> invariants;
        SmallVector<Value *, 64> codeMotionCandidates;
        BasicBlock *test = nullptr;
        BasicBlock *landingpad = nullptr;
        BasicBlock *preheader = nullptr;

        bool isDefOutsideLoop(Value *V, Loop *L)
        {

            for (auto BB : L->blocks())
            {
                for (auto &I : *BB)
                {
                    if (dyn_cast<Value>(&I) == V)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        bool isInvariant(Value *V)
        {
            return (std::find(invariants.begin(), invariants.end(), V) != invariants.end());
        }

        void getPreHeader(Loop *L)
        {
            BasicBlock *header = L->getHeader();
            for (auto predecessor : predecessors(header))
            {
                if (std::find(L->getBlocksVector().begin(), L->getBlocksVector().end(), predecessor) == L->getBlocksVector().end())
                {
                    // This predecessor is not part of loop, this is the preheader
                    preheader = predecessor;
                }
            }
        }

        void generateTest(Loop *L)
        {

            auto header = L->getHeader();
            auto branchInst = dyn_cast<BranchInst>(header->getTerminator());

            if (branchInst != nullptr && branchInst->isConditional())
            {

                auto conditionalVar = branchInst->getCondition();
                Value *cmp1 = nullptr, *cmp2 = nullptr, *newOp1 = nullptr, *newOp2 = nullptr;

                // conditionals are generated using cmp instructions, so we check
                // the operands of this conditional and see the variable of condition
                for (auto &I : *header)
                {
                    if (dyn_cast<Value>(&I) == conditionalVar)
                    {
                        if (!isa<CmpInst>(&I))
                            return;
                        auto cmpInst = dyn_cast<CmpInst>(&I);
                        cmp1 = cmpInst->getOperand(0);
                        cmp2 = cmpInst->getOperand(1);

                        outs() << "Operands are : " << getShortValueName(cmp1) << " " << getShortValueName(cmp2) << "\n";
                    }
                }

                newOp1 = cmp1;
                newOp2 = cmp2;

                for (auto &I : *header)
                {
                    if (Instruction::PHI == I.getOpcode())
                    {
                    }
                    if (dyn_cast<Value>(&I) == cmp1 || dyn_cast<Value>(&I) == cmp2)
                    {
                        if (Instruction::PHI == I.getOpcode())
                        {
                            auto phi = dyn_cast<PHINode>(&I);
                            auto entryOp = phi->getIncomingValueForBlock(preheader);
                            if (dyn_cast<Value>(&I) == cmp1)
                            {
                                newOp1 = entryOp;
                            }
                            if (dyn_cast<Value>(&I) == cmp2)
                            {
                                newOp2 = entryOp;
                            }
                        }
                    }
                }

                outs() << "New Operands are : " << getShortValueName(newOp1) << " " << getShortValueName(newOp2) << "\n";

                //  Remove phi instructions from the original header
                SmallVector<Instruction *, 10> toRemove;
                for (auto &I : *header)
                {
                    if (Instruction::PHI == I.getOpcode())
                    {
                        outs() << "We have PHI Node : " << I;
                        auto phinode = dyn_cast<PHINode>(&I);
                        for (auto &phival : phinode->incoming_values())
                        {
                            if (phival.get() != phinode->getIncomingValueForBlock(preheader))
                            {

                                phinode->replaceAllUsesWith(phival.get());
                                outs() << getShortValueName(phival.get()) << "\n";
                                break;
                            }
                        }
                        // phinode->eraseFromParent();
                        toRemove.push_back(&I);
                    }
                }

                // We have the two operands for cmp instruction now. Insert the instruction

                auto oldCmp = dyn_cast<CmpInst>(conditionalVar);

                IRBuilder<> *builder = new IRBuilder<>(test->getContext());
                builder->SetInsertPoint(test);

                auto newCmp = builder->CreateICmp(oldCmp->getPredicate(), newOp1, newOp2, "new-test");

                BranchInst *br = builder->CreateCondBr(newCmp, landingpad, L->getExitBlock());
                outs() << "test: " << *newCmp << "  " << *br << *test;

                outs() << "Removeable insts ";
                for (auto inst : toRemove)
                {
                    outs() << *inst;
                    inst->eraseFromParent();
                }
            }
        }

        void createLandingPad(Loop *L)
        {
            // We insert the landing pad blocks

            auto header = L->getHeader();
            auto preHeader = L->getLoopPredecessor();

            BasicBlock *newHeader = BasicBlock::Create(header->getContext(), "test", header->getParent(), header);

            BasicBlock *landingPad = BasicBlock::Create(header->getContext(), "landing-pad", header->getParent(), header);

            // Insert a branch terminator instruction in newHeader and landingPad

            BasicBlock *body;
            for (auto successor : successors(header))
            {
                if (std::find(L->getBlocks().begin(), L->getBlocks().end(), successor) != L->getBlocks().end())
                {
                    body = successor;
                }
            }
            BranchInst::Create(body, landingPad);

            // Fix the destination of branches in header and preheader
            auto preHeaderBranch = dyn_cast<BranchInst>(preHeader->getTerminator());
            for (size_t i = 0; i < preHeaderBranch->getNumSuccessors(); ++i)
            {
                if (preHeaderBranch->getSuccessor(i) == header)
                {
                    preHeaderBranch->setSuccessor(i, newHeader);
                }
            }
            outs() << "preheader: " << *preHeader << "\n";

            outs() << "header: " << *header << "\n";
            outs() << "newheader: " << *newHeader << "\n";

            outs() << "landingPad: " << *landingPad << "\n";
            test = newHeader;
            landingpad = landingPad;
        }

        virtual bool runOnLoop(Loop *L, LPPassManager &LPM) override
        {
            bool changed = true;

            int numInv = 0;

            for (auto BB : L->blocks())
            {
                outs() << BB->getName() << "\n";
            }

            while (changed)
            {
                int oldNum = numInv;

                for (auto BB : L->blocks())
                {

                    for (auto &I : *BB)
                    {
                        bool isInv = true;
                        if (!isAssignmentInstruction(I))
                        {
                            continue;
                        }

                        for (auto &op : I.operands())
                        {

                            auto operand = op.get();
                            if (!(isDefOutsideLoop(operand, L) || isa<Constant>(operand) || isInvariant(dyn_cast<Value>(operand))))
                            {
                                isInv = false;
                            }
                        }

                        if (isInv && (std::find(invariants.begin(), invariants.end(), dyn_cast<Value>(&I)) == invariants.end()))
                        {
                            numInv++;
                            invariants.push_back(dyn_cast<Value>(&I));
                        }
                    }
                }

                if (oldNum == numInv)
                {

                    changed = false;
                }
            }

            outs() << "Loop Invariants are :- "
                   << "\n";
            for (auto V : invariants)
            {

                outs() << *(dyn_cast<Instruction>(V)) << "\n";
            }

            getPreHeader(L);
            createLandingPad(L);
            generateTest(L);

            // Code Motion Candidates

            auto dominators = getAnalysis<DominatorsPass>().getDominators();

            SmallVector<BasicBlock *, 64> loopExitBlocks;
            L->getExitBlocks(loopExitBlocks);

            // Check for Code Motion candidates
            for (auto invariant : invariants)
            {
                bool canMove = true;
                for (auto loopExitBlock : loopExitBlocks)
                {
                    auto exitDom = dominators[loopExitBlock];
                    if (std::find(exitDom.begin(), exitDom.end(), dyn_cast<Instruction>(invariant)->getParent()) == exitDom.end())
                        canMove = false;
                }
            }

            return true;
        }

        virtual void getAnalysisUsage(AnalysisUsage &Info) const override
        {
            Info.addRequired<DominatorsPass>();
        };
    };
    char LICMPass::ID = 0;

    RegisterPass<LICMPass> Y("licm-rishi", "Loop Invariant Code Motion Pass");
    // Tell frontends to run the pass automatically.
    // static struct RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
    //                                        addDCEPass);

}