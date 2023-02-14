CC=clang
OPT=opt
PASS_DIR=../../LocalOpts/LocalOpts.so

constfold: ll
	${OPT} -mem2reg -S constfold.ll -o out.ll
	${OPT} -load ${PASS_DIR} -local-opts -S out.ll -o out.ll

ll:
	${CC} -O0 -emit-llvm -S constfold.c
clean:
	rm *.ll