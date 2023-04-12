#include <llvm/Analysis/LoopPass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
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
        BasicBlock *testBlock = nullptr;
        BasicBlock *landingpad = nullptr;
        BasicBlock *preheader = nullptr;
        BasicBlock *newHeader = nullptr;

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

        void getTestCondition(Loop *L)
        {

            auto headerBlock = L->getHeader();
            auto headerBranch = dyn_cast<BranchInst>(headerBlock->getTerminator());

            if (headerBranch == nullptr)
            {
                outs() << "Error: Header terminator not a branch inst";
                return;
            }

            if (headerBranch->isConditional() == false)
            {
                // TODO: Handle non conditional branch to body, so don't do anything
                return;
            }
            auto condition = dyn_cast<CmpInst>(headerBranch->getCondition());

            if (condition == nullptr)
            {
                outs() << "Conditional for header branch not a Cmp Inst";
                return;
            }

            auto operandOne = condition->getOperand(0);
            auto operandTwo = condition->getOperand(1);

            outs() << getShortValueName(operandOne) << " " << getShortValueName(operandTwo) << "\n";
            auto header = L->getHeader();
            for (auto &inst : *header)
            {
                if (dyn_cast<Value>(&inst) == operandOne)
                {
                    auto phi = dyn_cast<PHINode>(&inst);
                    if (phi == nullptr)
                    {
                        outs() << "Test block defines variable outside phi function\n";
                        return;
                    }
                    operandOne = phi->getIncomingValueForBlock(L->getLoopPreheader());
                }
                else if (dyn_cast<Value>(&inst) == operandTwo)
                {
                    auto phi = dyn_cast<PHINode>(&inst);
                    if (phi == nullptr)
                    {
                        outs() << "Test block defines variable outside phi function\n";
                        return;
                    }
                    operandTwo = phi->getIncomingValueForBlock(L->getLoopPreheader());
                }
            }

            // auto newConditional = CmpInst::Create(condition->getOpcode(), condition->getPredicate(), operandOne, operandTwo, "new-cmp", nullptr);
            auto term = testBlock->getTerminator();
            term->eraseFromParent();

            auto newConditional = CmpInst::Create(condition->getOpcode(), condition->getPredicate(), operandOne, operandTwo, "new-cmp", testBlock);
            auto newTerm = BranchInst::Create(landingpad, L->getExitBlock(), newConditional, testBlock);
        }

        void movePhisToBodyAndExit(Loop *L, BasicBlock *body)
        {
            // We have to move phi instructions to the body now, which is the new header
            auto header = L->getHeader();
            std::vector<PHINode *> instToMove;
            std::vector<Instruction *> newInstructions;
            std::vector<Value *> valueToReplace;
            llvm::ValueToValueMapTy vmap;
            for (auto &phi : header->phis())
            {
                instToMove.push_back(dyn_cast<PHINode>(&phi));
            }
            for (auto i : instToMove)
            {

                auto *newInst = dyn_cast<PHINode>(i->clone());
                newInst->setIncomingBlock(0, testBlock);
                newInst->setName("phi-" + i->getName());
                newInst->setIncomingBlock(1, header);
                newInst->insertBefore(&(L->getExitBlock()->front()));
                newInstructions.push_back(newInst);
                vmap[i] = newInst;

                valueToReplace.push_back(i);

                i->moveBefore(&body->front());
                i->setIncomingBlock(0, landingpad);
                i->setIncomingBlock(1, header);
            }

            for (auto *i : newInstructions)
            {
                llvm::RemapInstruction(i, vmap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
            }
            for (size_t i = 0; i < newInstructions.size(); i++)
            {
                Value *v = valueToReplace[i];

                for (auto user : v->users())
                {
                    if (auto I = dyn_cast<Instruction>(user))
                    {
                        if (std::find(L->blocks().begin(), L->blocks().end(), I->getParent()) == L->blocks().end())
                        {
                            for (int opNum = 0; opNum < I->getNumOperands(); ++opNum)
                            {
                                if (v == I->getOperand(opNum))
                                {
                                    I->setOperand(opNum, newInstructions[i]);
                                }
                            }
                        }
                    }
                }
            }
        }

        virtual bool runOnLoop(Loop *L, LPPassManager &LPM) override
        {

            outs() << "Performing Loop Invariant Code Motion\n";
            bool changed = true;

            int numInv = 0;

            while (changed)
            {
                int oldNum = numInv;

                for (auto BB : L->blocks())
                {

                    for (auto &I : *BB)
                    {
                        bool isInv = true;
                        if (!isAssignmentInstruction(I) || isa<PHINode>(&I))
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

            // We check if loop is while or do-while,
            // do-while loops do not require a landing pad

            if (dyn_cast<BranchInst>(L->getLoopLatch()->getTerminator())->isConditional())
            {
                outs() << "We have a do-while loop\n";

                auto doms = getAnalysis<DominatorsPass>().getDominators();
                auto exitDom = doms[L->getExitBlock()];
                for (auto inv : invariants)
                {
                    auto invariant = dyn_cast<Instruction>(inv);
                    if (std::find(exitDom.begin(), exitDom.end(), invariant->getParent()) != exitDom.end())
                    {
                        invariant->moveBefore(L->getLoopPreheader()->getTerminator());
                    }
                }

                return true;
            }

            preheader = L->getLoopPreheader();
            BasicBlock *body = nullptr;
            for (auto succ : successors(L->getHeader()))
            {
                if (succ != L->getExitBlock())
                {
                    body = succ;
                }
            }

            testBlock = SplitEdge(preheader, L->getHeader(), nullptr, nullptr, nullptr);
            testBlock->setName("test");
            landingpad = SplitEdge(testBlock, L->getHeader());
            landingpad->setName("landing-pad");

            // Code Motion Candidates

            getTestCondition(L);
            movePhisToBodyAndExit(L, body);

            L->moveToHeader(body);
            LoopInfo LI;
            if (auto parentLoop = L->getParentLoop())
            {
                parentLoop->addBasicBlockToLoop(testBlock, LI);
                parentLoop->addBasicBlockToLoop(landingpad, LI);
            }

            dyn_cast<BranchInst>(landingpad->getTerminator())->setSuccessor(0, body);
            auto dominators = getAnalysis<DominatorsPass>().getDominators();

            SmallVector<BasicBlock *, 64> loopExitBlocks;

            auto dfa = new DominatorsDFA(*testBlock->getParent());
            dfa->performDFA(*testBlock->getParent());

            L->getExitingBlocks(loopExitBlocks);

            // After Landing Pad insertion

            std::vector<PHINode *> removablePhis;

            // Check for Code Motion candidates
            for (auto invariant : invariants)
            {
                bool canMove = true;
                if (isa<PHINode>(invariant))
                {
                    canMove = false;
                }
                for (auto loopExitBlock : loopExitBlocks)
                {
                    auto exitDom = dfa->getDomList(loopExitBlock);

                    if (std::find(exitDom.begin(), exitDom.end(), dyn_cast<Instruction>(invariant)->getParent()) == exitDom.end())
                        canMove = false;
                }

                if (canMove)
                {

                    dyn_cast<Instruction>(invariant)->moveBefore(landingpad->getTerminator());

                    //  If any phi function in body used this value, replace its uses with this value
                    for (auto &phi : body->phis())
                    {
                        bool removePhi = false;

                        for (auto &val : phi.incoming_values())
                        {
                            if (val.get() == invariant)
                            {
                                removePhi = true;
                            }
                        }
                        if (removePhi)
                        {
                            phi.replaceAllUsesWith(invariant);
                            removablePhis.push_back(&phi);
                        }
                    }
                }
            }

            for (auto phi : removablePhis)
            {
                phi->eraseFromParent();
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