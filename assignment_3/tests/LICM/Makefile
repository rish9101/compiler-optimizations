all: test.ll

test.ll:
	clang -S -emit-llvm -fno-discard-value-names -Xclang -disable-O0-optnone -O0 test.c -o test.ll
	opt -S -mem2reg test.ll -o test.ll
	opt -S -load ../../LICM/liblicmpass.so -licm-rishi test.ll -o test-opt.ll
	
clean:
	rm *.ll