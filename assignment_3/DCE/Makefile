INC=-I/usr/local/include/ -I../
CC=clang
CXX=clang++
CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) $(INC) -g -O0 -fPIC

libdcepass.so: src/Pass.cc ../Dataflow/dataflow.o
	$(CXX) -dylib -shared $(CXXFLAGS) $^ -o $@

dataflow.o: ../Dataflow/dataflow.cpp ../Dataflow/dataflow.h

clean:
	rm -f *.o *~ *.so ../Dataflow/*.o

.PHONY: clean all
