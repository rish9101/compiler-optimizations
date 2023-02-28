CC=clang
OPT=opt
PASS_DIR=../../Dataflow/available.so

loop: ll
	${OPT} -load ${PASS_DIR} -available loop.ll -o out

ll:
	${CC} -O0 -emit-llvm -S loop.c
clean:
	rm *.ll