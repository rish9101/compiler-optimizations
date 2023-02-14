// ECE-5984 S21 Assignment 1: FunctionInfo.cpp
// PID: rishiranjan
////////////////////////////////////////////////////////////////////////////////
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraph.h"
#include <iostream>

using namespace llvm;

namespace
{

    class FunctionInfo : public FunctionPass
    {
    public:
        static char ID;
        FunctionInfo() : FunctionPass(ID) {}
        ~FunctionInfo() {}
        // We don't modify the program, so we preserve all analyses
        void getAnalysisUsage(AnalysisUsage &AU) const override
        {
            AU.setPreservesAll();
        }
        // Do some initialization.
        // This method run once by the compiler when the module is loaded
        bool doInitialization(Module &M) override
        {
            outs() << "Running Function Info Pass\n";
            return false;
        }

        // This method is called for every function in the module
        bool runOnFunction(Function &F) override
        {
            Module *mod = F.getParent();

            StringRef name = F.getName();

            u_int32_t numArguments = 0;
            // Count the number of arguments in every function
            for (auto argItr = F.arg_begin(); argItr != F.arg_end(); ++argItr)
            {
                ++numArguments;
            }

            u_int32_t numBasicBlocks = 0;
            u_int32_t numInstructions = 0;

            u_int32_t numAddSubInst = 0;
            u_int32_t numMulDivInst = 0;
            u_int32_t numBranchCondInst = 0;
            u_int32_t numBranchUncondInst = 0;

            // Count the number of basic blocks
            for (auto &bb : F)
            {
                ++numBasicBlocks;

                // Count the number and type of Instruction

                for (auto &inst : bb)
                {
                    ++numInstructions;
                    switch (inst.getOpcode())
                    {
                    case Instruction::Add:
                    case Instruction::Sub:
                    case Instruction::FAdd:
                    case Instruction::FSub:
                        ++numAddSubInst;
                        break;
                    case Instruction::Mul:
                    case Instruction::FMul:
                    case Instruction::SDiv:
                    case Instruction::UDiv:
                    case Instruction::FDiv:
                        ++numMulDivInst;
                        break;
                    case Instruction::Br:
                    {
                        auto *b = dyn_cast<BranchInst>(&inst);
                        if (b->isConditional())
                        {
                            ++numBranchCondInst;
                        }
                        else
                        {
                            ++numBranchUncondInst;
                        }

                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            CallGraph cg = CallGraph(*mod);

            // Extract the CallGraphNode from the Call Graph
            auto funcNode = cg.getOrInsertFunction(&F);
            int numOfDirectCalls = funcNode->getNumReferences();

            outs() << "--------------------------------";
            outs() << "Function Name: " << name << "\n";
            outs() << "Number of arguments: " << numArguments << "\n";
            outs() << "Number of Basic Blocks: " << numBasicBlocks << "\n";
            outs() << "Number of Instructions: " << numInstructions << "  "
                   << "\n";
            outs() << "Number of Add/Sub Instructions: " << numAddSubInst << "  "
                   << "\n";
            outs() << "Number of Mul/Div Instructions: " << numMulDivInst << "  "
                   << "\n";
            outs() << "Number of Branch (Cond) Instructions: " << numBranchCondInst << "  "
                   << "\n";
            outs() << "Number of Branch (Uncond) Instructions: " << numBranchUncondInst << "  "
                   << "\n";
            outs() << "Number of Calls to  the function in the same module " << numOfDirectCalls - 1 << "  "
                   << "\n\n";
            return false;
        }
    };
}

// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.
char FunctionInfo::ID = 0;
static RegisterPass<FunctionInfo> X("function-info", "5984: Function Information",
                                    false, false);