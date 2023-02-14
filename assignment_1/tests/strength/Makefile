CC=clang
OPT=opt
PASS_DIR=../../LocalOpts/LocalOpts.so

strength: ll
	${OPT} -mem2reg -S strength.ll -o out.ll
	${OPT} -load ${PASS_DIR} -local-opts -S out.ll -o out.ll

ll:
	${CC} -O0 -emit-llvm -S strength.c

clean:
	rm *.ll