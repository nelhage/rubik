override CXXFLAGS += -std=c++14 -Wall -Werror -Wno-unused-parameter
LINK.o := $(LINK.cc)

all: rubik_test

rubik.o: rubik.h
rubik_test.o: rubik.h
rubik_bench.o: rubik.h

rubik_test: rubik_test.o rubik.o
rubik_bench: rubik_bench.o rubik.o

test: rubik_test
	./rubik_test

clean:
	rm -f rubik.o rubik_test.o rubik_test

.PHONY: test clean
