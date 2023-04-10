
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLASSICAL_DATAFLOW_H__
#define __CLASSICAL_DATAFLOW_H__

#include <stdio.h>
#include <iostream>
#include <queue>
#include <vector>
#include <tuple>

#include "llvm/IR/Instructions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/CFG.h"

namespace llvm
{

    /**
     * @brief Enum for the direction of Dataflow Analysis
     * Can be Forwards or Backwards
     *
     */
    enum Direction
    {
        FORWARDS,
        BACKWARDS
    };

    /**
     * @brief The abstract Dataflow class - Every DFA inherits from this class
     *
     *
     * @tparam ValueType - The type of the domain (e.g variable, expression) for the DFA
     */
    template <class ValueType>
    class Dataflow
    {
    private:
        std::vector<ValueType> domain;
        Direction direction;

    public:
        ValueMap<BasicBlock *, BitVector> InBB;
        ValueMap<BasicBlock *, BitVector> OutBB;
        ValueMap<BasicBlock *, BitVector> GenBB;
        ValueMap<BasicBlock *, BitVector> KillBB;

        /**
         * @brief Get the Domain object
         *
         * @return std::vector<ValueType>&
         */
        std::vector<ValueType> &getDomain() { return domain; }

        /**
         * @brief Get the Direction object
         *
         * @return Direction
         */
        Direction getDirection()
        {
            return direction;
        };

        ValueMap<BasicBlock *, BitVector> &getInBB()
        {
            return InBB;
        }

        ValueMap<BasicBlock *, BitVector> &getOutBB()
        {
            return OutBB;
        }

        /**
         * @brief Construct a new Dataflow object
         *
         * @param direction
         */
        Dataflow(Direction direction) : direction(direction){};

        /**
         * @brief This is the transfer function as defined in a DFA
         * @param BasicBlock - BasicBlock for which we are calculating the transfer function
         * @return BitVector - The BitVector produced by the transferFunc
         */
        virtual BitVector transferFunc(BasicBlock &) = 0;

        /**
         * @brief This is the meet operator of the meet semi-lattice for the DFA
         * @param BasicBlock - BasicBlock for which we are calculating the meet operator
         * @return BitVector
         */
        virtual BitVector meetOp(std::vector<BitVector>) = 0;

        /**
         * @brief Get the Gen And Kill Set object
         * @param BasicBlock - BasicBlock for which we are computing the Gen and Kill Set
         * @return std::pair<BitVector, BitVector>
         */
        virtual std::pair<BitVector, BitVector> getGenAndKillSet(BasicBlock &) = 0;

        /**
         * @brief The method which performs the iterative DFA
         * @param Function - Function for which we are performing DFA
         *
         */
        void performDFA(Function &F)
        {

            // ---------------------
            // ITERATIVE STEP
            // ---------------------
            int i = 0;

            bool changed = true;
            while (changed == true)
            {
                i = 0;
                bool newChanged = false;
                for (auto &BB : F)
                {
                    auto oldOut = OutBB[&BB];

                    // ---------------------
                    // CALCULATE ENTRY WITH MEET OPERATOR
                    // ---------------------

                    // Calculating IN[BB] = meetOp(Out[P]) for every predecessor P of BB

                    if (direction == FORWARDS)
                    {
                        InBB[&BB].reset();
                        std::vector<BitVector> meetCandidates;
                        for (auto pred : predecessors(&BB))
                        {
                            meetCandidates.push_back(OutBB[pred]);
                        }
                        InBB[&BB] |= meetOp(meetCandidates);
                    }
                    else
                    {

                        OutBB[&BB] &= BitVector(256, false);
                        std::vector<BitVector> meetCandidates;
                        for (auto succ : successors(&BB))
                        {
                            meetCandidates.push_back(InBB[succ]);
                        }
                        OutBB[&BB] |= meetOp(meetCandidates);
                    }
                    // ---------------------
                    // CALCULATING OUT (IN) FOR FORWARD (BACKWARD)
                    // DFA USING THE TRANSFER FUNCTION
                    // ---------------------

                    // Calculating GenSet and KillSet

                    BitVector genSet, killSet;
                    std::tie(genSet, killSet) = getGenAndKillSet(BB);

                    if (direction == FORWARDS)
                    {
                        OutBB[&BB] &= BitVector(256, false);
                        OutBB[&BB] |= transferFunc(BB);

                        if (oldOut != OutBB[&BB])
                        {
                            newChanged = true;
                        }
                    }
                    else
                    {
                        auto oldIn = InBB[&BB];
                        InBB[&BB] &= BitVector(256, false);
                        InBB[&BB] |= transferFunc(BB);

                        if (oldIn != InBB[&BB])
                        {
                            newChanged = true;
                        }
                    }
                    ++i;
                }
                changed = newChanged;
            }
        }
    };
}
#endif
