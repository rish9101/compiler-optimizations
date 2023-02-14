CC=clang
OPT=opt
PASS_DIR=../../LocalOpts/LocalOpts.so

algebraic: ll
	${OPT} -mem2reg -S algebraic.ll -o out.ll
	${OPT} -load ${PASS_DIR} -local-opts -S out.ll -o out.ll

ll:
	${CC} -disable-O0-optnone -O0 -emit-llvm -S algebraic.c

	
clean:
	rm *.ll
