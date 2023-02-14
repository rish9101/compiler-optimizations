// ECE-5984 S21 Assignment 1: LocalOpts.cpp
// PID: rishiranjan
////////////////////////////////////////////////////////////////////////////////
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <iostream>
#include <set>
#include <utility>
using namespace llvm;

namespace
{
    // Helpler function to check if the instruction is an Add Inst
    bool isAddInst(unsigned int opcode)
    {
        bool ret;
        switch (opcode)
        {
        case Instruction::Add:
            ret = true;
            break;
        default:
            ret = false;
            break;
        }

        return ret;
    }

    // Helpler function to check if the instruction is a Sub Inst
    bool isSubInst(unsigned int opcode)
    {
        bool ret;
        switch (opcode)
        {
        case Instruction::Sub:
            ret = true;
            break;
        default:
            ret = false;
            break;
        }

        return ret;
    }

    // Helpler function to check if the instruction is a Mul Inst
    bool isMulInst(unsigned int opcode)
    {
        bool ret;
        switch (opcode)
        {
        case Instruction::Mul:
            ret = true;
            break;
        default:
            ret = false;
            break;
        }

        return ret;
    }

    // Helpler function to check if the instruction is a Division Inst
    bool isDivInst(unsigned int opcode)
    {
        bool ret;
        switch (opcode)
        {
        case Instruction::SDiv:
            ret = true;
            break;
        case Instruction::UDiv:
            ret = true;
            break;
        default:
            ret = false;
            break;
        }

        return ret;
    }

    /**
     * @brief We implement the following algebraic identities in this optimization
     * x + 0 = 0 + x = x
     * x - 0 = x
     * x * 1 = 1 * x = x
     * x / x = 1
     * x - x = 0
     * x * 0 = 0 * x = 0
     *
     * @param inst
     */
    bool optAlgebraicIdentities(Instruction &inst)
    {
        // First identity x + 0 = 0 + x = x
        bool isAdd = isAddInst(inst.getOpcode());

        if (isAdd == true)
        {
            for (size_t i = 0; i < inst.getNumOperands(); ++i)
            {
                auto operand = dyn_cast<ConstantInt>(inst.getOperand(i));
                if (operand && operand->equalsInt(0))
                {
                    outs() << "Found identity ( x + 0 ) \n";
                    // Removing all the uses

                    auto val = (i == 0) ? inst.getOperand(1) : inst.getOperand(0);
                    inst.replaceAllUsesWith(val);

                    return true;
                }
            }
        }

        if (isSubInst(inst.getOpcode()) == true)
        {

            auto operand = dyn_cast<ConstantInt>(inst.getOperand(1));
            // Identity check for y = x - 0
            if (operand && operand->equalsInt(0))
            {
                outs() << "Found (x - 0) identity \n";
                auto val = inst.getOperand(0);
                inst.replaceAllUsesWith(val);
                return true;
            }
        }

        if (isMulInst(inst.getOpcode()) == true)
        {
            for (size_t i = 0; i < inst.getNumOperands(); ++i)
            {
                auto operand = dyn_cast<ConstantInt>(inst.getOperand(i));

                // Identity check for y = x * 1
                if (operand && operand->equalsInt(1))
                {
                    outs() << "Found (x * 1) identity \n";
                    // Removing all the uses

                    auto val = (i == 0) ? inst.getOperand(1) : inst.getOperand(0);
                    inst.replaceAllUsesWith(val);

                    return true;
                }

                // Identity check for x * 0
                else if (operand && operand->equalsInt(0))
                {
                    outs() << "Found (x * 0) identity \n";

                    auto val = ConstantInt::get(inst.getType(), 0);
                    inst.replaceAllUsesWith(val);
                    return true;
                }
            }
        }

        if (isDivInst(inst.getOpcode()) == true)
        {
            // Identity check for y = x / 1
            auto operand = dyn_cast<ConstantInt>(inst.getOperand(1));
            if (operand->equalsInt(1))
            {
                inst.replaceAllUsesWith(ConstantInt::get(inst.getType(), 1));
            }
        }

        return false;
    }

    // This is a helper function to get the result of a constant expression
    Constant *getConstExprResult(ConstantInt *firstOperand, ConstantInt *secondOperand, unsigned int opcode, Type *type)
    {
        switch (opcode)
        {
        case Instruction::Add:
            return ConstantInt::get(type, firstOperand->getValue() + secondOperand->getValue());
            break;

        case Instruction::Sub:
            return ConstantInt::get(type, firstOperand->getValue() - secondOperand->getValue());
            break;

        case Instruction::Mul:
            return ConstantInt::get(type, firstOperand->getValue() * secondOperand->getValue());
            break;
        case Instruction::SDiv:
            return ConstantInt::get(type, firstOperand->getValue().sdiv(secondOperand->getValue()));
            break;
        case Instruction::UDiv:
            return ConstantInt::get(type, firstOperand->getValue().udiv(secondOperand->getValue()));
            break;

        default:
            break;
        }
    }

    // This map holds all the constants we have encountered so far
    ValueMap<Value *, ConstantInt *> consts;

    // Constant folding optimization
    bool constFolding(Instruction &inst)
    {

        if (inst.getOpcode() == Instruction::Store)
        {
            // We have a store operation
            auto storeVal = dyn_cast<ConstantInt>(inst.getOperand(0));
            if (storeVal)
            {
                auto constantVar = inst.getOperand(1);
                // We insert the variable and its constant value into the consts map
                if (consts.find(constantVar) != consts.end())
                {
                    consts.erase(constantVar);
                }
                consts.insert(std::pair<Value *, ConstantInt *>(constantVar, storeVal));

                return true;
            }
        }
        else if (inst.getOpcode() == Instruction::Load)
        {

            auto loadSource = inst.getOperand(0);
            auto possibleConst = consts.find(loadSource);
            if (possibleConst != consts.end())
            {
                inst.replaceAllUsesWith(possibleConst->second);
                return true;
            }
        }
        else if (Instruction::isBinaryOp(inst.getOpcode()) == true)
        {
            auto firstOperand = dyn_cast<ConstantInt>(inst.getOperand(0));
            auto secondOperand = dyn_cast<ConstantInt>(inst.getOperand(1));

            if (firstOperand && secondOperand)
            {
                // Both the operands are constant
                auto val = getConstExprResult(firstOperand, secondOperand, inst.getOpcode(), inst.getType());
                inst.replaceAllUsesWith(val);
                return true;
            }
        }
        return false;
    }

    // Helper function to check if a number is a power of two
    bool isPowerOfTwo(unsigned int num)
    {
        return (num & (num - 1)) == 0 ? true : false;
    }

    /**
     * @brief This function implements the strength reduction optimization
     *
     * @param inst
     * @return true
     * @return false
     */
    bool strengthReduction(Instruction &inst)
    {
        bool isMul = isMulInst(inst.getOpcode());

        if (isMul == true)
        {
            for (size_t i = 0; i < inst.getNumOperands(); ++i)
            {
                auto operand = dyn_cast<ConstantInt>(inst.getOperand(i));
                auto var = (i == 0) ? inst.getOperand(1) : inst.getOperand(0);

                if (operand && isPowerOfTwo(operand->getSExtValue()))
                {
                    uint64_t exp = (uint64_t)log2(operand->getSExtValue());
                    auto a = ConstantInt::get(operand->getType(), exp);
                    // We replace the multiply instruction with left shift
                    inst.replaceAllUsesWith(BinaryOperator::Create(
                        Instruction::Shl, var, a, "", &inst));

                    return true;
                }
            }
        }

        bool isDiv = isDivInst(inst.getOpcode());

        if (isDiv == true)
        {

            auto operand = dyn_cast<ConstantInt>(inst.getOperand(1));
            auto var = inst.getOperand(0);

            if (operand && isPowerOfTwo(operand->getSExtValue()))
            {
                uint64_t exp = (uint64_t)log2(operand->getSExtValue());
                auto a = ConstantInt::get(operand->getType(), exp);
                // We replace the divide instruction with right shift
                inst.replaceAllUsesWith(BinaryOperator::Create(
                    Instruction::AShr, var, a, "", &inst));

                return true;
            }
        }
        return false;
    }

    class LocalOpts : public FunctionPass
    {
    private:
        unsigned int numConstFolding = 0;
        unsigned int numAlgebraicIdentity = 0;
        unsigned int numStrengthReduction = 0;

    public:
        static char ID;
        LocalOpts() : FunctionPass(ID) {}
        ~LocalOpts() {}
        // We don't modify the program, so we preserve all analyses
        void getAnalysisUsage(AnalysisUsage &AU) const override
        {
            AU.setPreservesAll();
        }
        // Do some initialization.
        // This method run once by the compiler when the module is loaded
        bool doInitialization(Module &M) override
        {
            outs() << "Running Local Opts Pass\n";
            return false;
        }

        // This method is called for every function in the module
        bool runOnFunction(Function &F) override
        {

            std::set<Instruction *> WorkList;

            for (auto &bb : F)
            {
                SmallVector<Instruction *, 10> toRemove;
                for (auto &inst : bb)
                {
                    bool remove = constFolding(inst);

                    if (!remove)
                    {
                        remove = strengthReduction(inst);
                        if (!remove)
                        {
                            remove = optAlgebraicIdentities(inst);
                            if (remove)
                                ++numAlgebraicIdentity;
                        }
                        else
                        {
                            ++numStrengthReduction;
                        }
                    }
                    else
                    {
                        ++numConstFolding;
                    }
                    if (remove)
                        toRemove.push_back(&inst);
                }

                for (size_t i = 0; i < toRemove.size(); ++i)
                {

                    toRemove[i]->eraseFromParent();
                }
            }

            outs() << "Algebraic Identity : " << numAlgebraicIdentity << "\n";
            outs() << "Strength Reduction : " << numStrengthReduction << "\n";
            outs() << "Constant Folding : " << numConstFolding << "\n";

            return true;
        }
    };
};

// LLVM uses the address of this static member to identify the pass, so the
// initialization value is unimportant.
char LocalOpts::ID = 0;
static RegisterPass<LocalOpts> X("local-opts", "5544: Local Optimizations on the function",
                                 false, false);