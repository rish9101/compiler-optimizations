# Assignment 1

This folder contains two compiler passes solving Assignment 1 of CS5544

## FunctionInfo Pass

This pass collects and prints basic information about the pass

To build this pass

```bash
cd ./FunctionInfo
make
```

This generates the function pass in the form of shared library `FunctionInfo.so`
## LocalOpts Pass
This Pass performs simple local optimizations like constant folding, algebraic identity optimisation and strength reduction.

To build this pass

```bash
cd ./LocalOpts
make
```
This generates the function pass in the form of shared library `LocalOpts.so`
## Tests

The tests folder contains four test programs for the passes.
- algebraic (test for local opt pass)
- constfold (test for local opt pass)
- loop (test for function info pass)
- strength (test for local opt pass)

Every folder has a Makefile which generate the optimized LLVM IR in the filename `out.ll`
To build any test

```bash
cd ./tests/<SomeTest>
make
```