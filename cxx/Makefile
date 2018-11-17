override CXXFLAGS += -std=c++14 -Wall -Werror -Wno-unused-parameter -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2
LINK.o := $(LINK.cc)

EXES=rubik_test rubik_bench

all: rubik_test

rubik.o: rubik.h
rubik_test.o: rubik.h
rubik_bench.o: rubik.h tables.h

rubik_test: test_main.o rubik.o
rubik_bench: rubik.o
rubik_scratch: rubik.o tables.o

test: rubik_test
	./rubik_test

bench: rubik_bench
	./rubik_bench

scratch: rubik_scratch
	./rubik_scratch

clean:
	rm -f $(wildcard *.o) $(EXES)

.PHONY: test bench clean scratch
