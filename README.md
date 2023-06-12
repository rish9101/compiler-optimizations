# Compiler Optimizarions

This repository hosts implementations of some of the most popular compiler optimizations. These are built as LLVM Passes.
The transformation passes implemented are:
1. Faint analysis based dead code elimination
2. Loop Invariant Code motion
3. Local Constant Folding
4. Algebraic Identity Optimization
5. Simple strength reduction

Analysis Passses implemented are:
1. Dominators analysis
2. Available Expression analysis
3. Faint Analysis

A lot of these analysis passes use a dataflow analysis (both seperable and non-seperable) based approach. A simple framework for implementing a Dataflow analysis is also implemented.

