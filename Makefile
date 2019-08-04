DBGFLAGS:=-fprofile-arcs -ftest-coverage -g -pg
DBGFLAGS:=-g
CFLAGS:=-std=c99 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wvla $(DBGFLAGS)
CXXFLAGS:=-std=c++11 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wvla $(DBGFLAGS)
LDFLAGS:=-O3
LDLIBS:=-lm -lcjson -lGL -lGLEW -lglfw
TARGETS:=test example simulation gui

all: $(TARGETS)

example: body.o orbit.o recipes.o util.o load.o lambert.o
test: body.o orbit.o util.o load.o recipes.o lambert.o
simulation: body.o orbit.o util.o load.o recipes.o lambert.o
gui: mesh.o texture.o shaders.o body.o orbit.o load.o util.o

# handle include dependencies
-include $(wildcard *.d)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) -MM -o $*.d $<
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	$(CXX) $(CXXFLAGS) -MM -o $*.d $<

clean:
	rm -f *.o *.d *.gcda *.gcno *.gcov

.PHONY: all clean
