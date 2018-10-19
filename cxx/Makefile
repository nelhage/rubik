override CXXFLAGS += -std=c++14 -Wall -Werror -Wno-unused-parameter
LINK.o := $(LINK.cc)

EXES=rubik_test rubik_bench

all: rubik_test

rubik.o: rubik.h
rubik_test.o: rubik.h
rubik_bench.o: rubik.h

rubik_test: rubik_test.o test_main.o rubik.o
rubik_bench: rubik_bench.o rubik.o

test: rubik_test
	./rubik_test

bench: rubik_bench
	./rubik_bench

clean:
	rm -f $(wildcard *.o) $(EXES)

.PHONY: test bench clean
