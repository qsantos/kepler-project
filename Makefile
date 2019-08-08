DBGFLAGS:=-fprofile-arcs -ftest-coverage -g -pg
DBGFLAGS:=-g
CFLAGS:=-std=c99 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wvla $(DBGFLAGS)
CXXFLAGS:=-std=c++11 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wvla $(DBGFLAGS)
LDFLAGS:=-O3
LDLIBS:=-lm -lcjson -lGL -lGLEW -lglfw
TARGETS:=test example simulation gui

ifeq ($(PLATFORM),win32)
	export C_INCLUDE_PATH:=/usr/local/x86_64-w64-mingw32/include/
	export CPLUS_INCLUDE_PATH:=/usr/local/x86_64-w64-mingw32/include/
	# Cross compilers such as MinGW on Linux ignore LIBRARY_PATH...
	# export LIBRARY_PATH:=/usr/local/x86_64-w64-mingw32/lib/
	CC=x86_64-w64-mingw32-gcc
	CXX=x86_64-w64-mingw32-g++
	CFLAGS:=$(CFLAGS) -D__USE_MINGW_ANSI_STDIO=1
	CXXFLAGS:=$(CXXFLAGS) -D__USE_MINGW_ANSI_STDIO=1
	LDFLAGS:=$(LDFLAGS) -static
	LDLIBS:=-L/usr/local/x86_64-w64-mingw32/lib -lm -lcjson -lglfw3 -lgdi32 -lopengl32 -lglew32
endif

all: $(TARGETS)

example: body.o orbit.o recipes.o util.o load.o lambert.o
test: body.o orbit.o util.o load.o recipes.o lambert.o
simulation: body.o orbit.o util.o load.o recipes.o lambert.o
gui: mesh.o texture.o shaders.o cubemap.o text_panel.o body.o orbit.o load.o util.o

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
