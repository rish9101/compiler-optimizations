# HEAP LIVENESS ANALYSIS

## Access Path based analysis

### Intraprocedural analysis

We only deal with

a = malloc()
and 
a->f1 = b->f2
calls.

Thus the IR instructions considered are 
1. bitcast(called after malloc calls)
2. getelementptr (used to access fields of a struct or array)
3. load/store (can be an assignment statement OR a use statement, depending on the operands)


### Benchmarks (Possible)

1. https://www.nas.nasa.gov/software/npb.html
2. https://parsec.cs.princeton.edu/
