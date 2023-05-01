all: test-interprocedural


linked-list-test: linked-list-test.c
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 ./linked-list-test.c -o ./linked-list-test.ll
	opt -S -mem2reg ./linked-list-test.ll -o ./linked-list-test.ll
	opt -S -load ../build/libmemorygraph.so -load ../build/libheapliveness.so -heap-liveness ./linked-list-test.ll -o ./linked-list-test.ll
	# opt -S -load ../build/libmemorygraph.so -memory-graph ./linked-list-test.ll -o ./linked-list-test.ll
test: test.c
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 ./test.c -o ./test.ll
	opt -S -mem2reg ./test.ll -o ./test.ll
	opt -S -load ../build/libmemorygraph.so -load ../build/libheapliveness.so -heap-liveness ./test.ll -o ./test.ll
	# opt -S -load ../build/libmemorygraph.so -memory-graph ./test.ll -o ./test.ll
test-interprocedural: test-interprocedural.c
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 ./test-interprocedural.c -o ./test-interprocedural.ll
	opt -S -mem2reg ./test-interprocedural.ll -o ./test-interprocedural.ll
	opt -S -load ../build/libmemorygraph.so -load ../build/libheapliveness.so -heap-liveness ./test-interprocedural.ll -o ./test-interprocedural.ll
	# opt -S -load ../build/libmemorygraph.so -memory-graph ./test-interprocedural.ll -o ./test-interprocedural.ll

clean:
	rm *.ll