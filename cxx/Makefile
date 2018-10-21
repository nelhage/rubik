override CXXFLAGS += -std=c++14 -Wall -Werror -Wno-unused-parameter -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2
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
