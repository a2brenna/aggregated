INCLUDE_DIR=$(shell echo ~)/local/include
LIBRARY_DIR=$(shell echo ~)/local/lib
DESDTIR=/
PREFIX=/usr
GITREF=\"$(shell git log | head -n 1 | awk '{ print $$2; }')\"
GITSTATUS=\"$(shell echo 'if [ "x$$(git status -s)" == "x" ]; then echo "clean"; else echo "dirty"; fi' | bash)\"

CXX=g++
CXXFLAGS=-DGITREF=${GITREF} -DGITSTATUS=${GITSTATUS} -L${LIBRARY_DIR} -I${INCLUDE_DIR} -march=native -O3 -flto -std=c++14 -fPIC -Wall -Wextra -fopenmp -fno-omit-frame-pointer -g

all: aggregated libaggregate.so libaggregate.a

aggregated: src/aggregated.cc
	${CXX} ${CXXFLAGS} -o aggregated src/aggregated.cc -lboost_program_options

libaggregate.so: client.o
	${CXX} ${CXXFLAGS} -shared -Wl,-soname,libaggregate.so -o libaggregate.so client.o

libaggregate.a: client.o
		ar rcs libaggregate.a client.o

client.o: src/client.cc
	${CXX} ${CXXFLAGS} -c src/client.cc -o client.o

clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.so
	rm -f aggregated
