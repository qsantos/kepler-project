#ifndef LOAD_HPP
#define LOAD_HPP

extern "C" {
#include "dict.h"
}

char* load_file(const char* filename);

int parse_bodies(Dict* bodies, const char* json);

int load_bodies(Dict* bodies, const char* filename);

void unload_bodies(Dict* bodies);

#endif
