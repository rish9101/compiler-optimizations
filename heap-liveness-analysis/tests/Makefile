test: test.c
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 ./test.c -o ./test.ll
	opt -S -mem2reg ./test.ll -o ./test.ll
	opt -S -load ../build/libheapliveness.so -heap-liveness ./test.ll -o ./test.ll

clean:
	rm *.ll