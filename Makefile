DBGFLAGS:=-fprofile-arcs -ftest-coverage -g -pg
DBGFLAGS:=-g
CFLAGS:=-std=c99 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wvla $(DBGFLAGS)
CXXFLAGS:=-std=c++11 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wvla $(DBGFLAGS)
LDFLAGS:=-O3
LDLIBS:=-lm -lcjson
TARGETS:=test example

all: $(TARGETS)

example: dict.o body.o orbit.o recipes.o load.o lambert.o
test: dict.o body.o orbit.o load.o recipes.o lambert.o

# handle include dependencies
-include $(wildcard *.d)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) -MM -o $*.d $<

clean:
	rm -f *.o *.d *.gcda *.gcno *.gcov

.PHONY: all clean
