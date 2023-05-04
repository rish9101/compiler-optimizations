# HEAP LIVENESS ANALYSIS

This is a compile-time instrumentation project to insert calls to free as soon as a heap object is dead. It is based on liveness-based heap analysis approached mentioned in the "Data Flow Analysis - Theory and Practice" book. It is still a bit limited in what programs it can handle but does not affect correctness of a program for any case

## How to build

### Requirements
- cmake (tested on version 3.16.3)
- llvm (tested on version 10.0.0)
- valgrind-3.15.0


### To build the compiler passes.
```bash
mkdir build
cd build
cmake ..
make
```

### To run the tests
```bash
cd tests
make <correctness-check-benchmark/interprocedural-benchmark/intraprocedural-linked-list-benchmark>
```

### Benchmarks (Possible)

1. https://www.nas.nasa.gov/software/npb.html
2. https://parsec.cs.princeton.edu/
