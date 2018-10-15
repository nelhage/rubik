CXXFLAGS = -std=c++14 -Wall -Werror
LINK.o := $(LINK.cc)

rubik.o: rubik.h
rubik_test.o: rubik.h

rubik_test: rubik_test.o rubik.o

all: rubik_test

test: rubik_test
	./rubik_test

.PHONY: test
