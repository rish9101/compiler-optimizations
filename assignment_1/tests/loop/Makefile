CC=clang
OPT=opt
PASS_DIR=../../FunctionInfo/FunctionInfo.so

loop: ll
	${OPT} -enable-new-pm=0 -load ${PASS_DIR} -function-info loop.ll -o out

ll:
	${CC} -O0 -emit-llvm -S loop.c
clean:
	rm *.ll