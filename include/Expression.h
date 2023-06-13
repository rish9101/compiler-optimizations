
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Common.h"

#include <string>
#include <vector>

namespace llvm
{

class Expression
{
  public:
    Value *v1;
    Value *v2;
    Instruction::BinaryOps op;
    Expression(Instruction *I);
    bool operator==(const Expression &e2) const;
    bool operator<(const Expression &e2) const;
    std::string toString() const;
};

void printSet(std::vector<Expression> x);
} // namespace llvm
