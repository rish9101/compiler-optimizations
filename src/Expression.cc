#include "Expression.h"

namespace llvm
{
// The Expression class is provided here to help
// you work with the expressions we'll be concerned
// about for the Available Expression analysis
Expression::Expression(Instruction *I)
{
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(I))
    {
        this->v1 = BO->getOperand(0);
        this->v2 = BO->getOperand(1);
        this->op = BO->getOpcode();
    }
    else
    {
        errs() << "We're only considering BinaryOperators\n";
    }
}

// For two expressions to be equal, they must
// have the same operation and operands.
bool Expression::operator==(const Expression &e2) const
{
    return this->v1 == e2.v1 && this->v2 == e2.v2 && this->op == e2.op;
}

// Less than is provided here in case you want
// to use STL maps, which use less than for
// equality checking by default
bool Expression::operator<(const Expression &e2) const
{
    if (this->v1 == e2.v1)
    {
        if (this->v2 == e2.v2)
        {
            if (this->op == e2.op)
            {
                return false;
            }
            else
            {
                return this->op < e2.op;
            }
        }
        else
        {
            return this->v2 < e2.v2;
        }
    }
    else
    {
        return this->v1 < e2.v1;
    }
}

// A pretty printer for Expression objects
// Feel free to alter in any way you like
std::string Expression::toString() const
{
    std::string op = "?";
    switch (this->op)
    {
    case Instruction::Add:
    case Instruction::FAdd:
        op = "+";
        break;
    case Instruction::Sub:
    case Instruction::FSub:
        op = "-";
        break;
    case Instruction::Mul:
    case Instruction::FMul:
        op = "*";
        break;
    case Instruction::UDiv:
    case Instruction::FDiv:
    case Instruction::SDiv:
        op = "/";
        break;
    case Instruction::URem:
    case Instruction::FRem:
    case Instruction::SRem:
        op = "%";
        break;
    case Instruction::Shl:
        op = "<<";
        break;
    case Instruction::AShr:
    case Instruction::LShr:
        op = ">>";
        break;
    case Instruction::And:
        op = "&";
        break;
    case Instruction::Or:
        op = "|";
        break;
    case Instruction::Xor:
        op = "xor";
        break;
    }
    return getShortValueName(v1) + " " + op + " " + getShortValueName(v2);
}

// Silly code to print out a set of expressions in a nice
// format
void printSet(std::vector<Expression> x)
{
    bool first = true;
    outs() << "{";

    for (std::vector<Expression>::iterator it = x.begin(), iend = x.end(); it != iend; ++it)
    {
        if (!first)
        {
            outs() << ", ";
        }
        else
        {
            first = false;
        }
        outs() << (it->toString());
    }
    outs() << "}\n";
}

} // namespace llvm
