all: dominator.ll

dominator.ll: dominator.c
	clang -S -emit-llvm -fno-discard-value-names -O0 dominator.c -o dominator.ll
	opt -S -load ../../Dominators/libdominators.so -dominators ./dominator.ll -o dominator.ll

test.ll: dominator.c
	clang -S -emit-llvm -fno-discard-value-names -O0 test.c -o test.ll
	opt -S -load ../../Dominators/libdominators.so -dominators ./test.ll -o test.ll


clean:
	rm *.ll