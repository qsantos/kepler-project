CCFLAGS+=-O3 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wvla -g
CFLAGS+=$(CCFLAGS) -std=c99 -Wstrict-prototypes
CXXFLAGS+=$(CCFLAGS) -std=c++11
LDFLAGS+=-O3
LDLIBS:=-lm -lcjson -lGL -lGLEW -lglfw -lstdc++
TARGETS:=test example simulation gui

ifeq ($(PLATFORM),win32)
	export C_INCLUDE_PATH:=/usr/local/x86_64-w64-mingw32/include/
	export CPLUS_INCLUDE_PATH:=/usr/local/x86_64-w64-mingw32/include/
	CC:=x86_64-w64-mingw32-gcc
	CXX:=x86_64-w64-mingw32-g++
	CCFLAGS+=-D__USE_MINGW_ANSI_STDIO=1
	LDFLAGS+=-static -mwindows
	# NOTE: MinGW on Linux ignores LIBRARY_PATH
	LDLIBS:=-L/usr/local/x86_64-w64-mingw32/lib -lstdc++ -lm -lcjson -lglfw3 -lopengl32 -lglew32
endif

all: $(TARGETS)

example: example.o body.o orbit.o recipes.o util.o load.o lambert.o
test: test.o body.o orbit.o util.o load.o recipes.o lambert.o
simulation: simulation.o body.o orbit.o util.o load.o recipes.o lambert.o
gui: gui.o mesh.o texture.o shaders.o cubemap.o text_panel.o body.o orbit.o load.o util.o

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

destroy: clean
	rm -f $(TARGETS)

.PHONY: all clean destroy
