#ifndef LOAD_HPP
#define LOAD_HPP

#include "body.hpp"

#include <map>
#include <string>

using Dict = std::map<std::string, CelestialBody*>;

int parse_bodies(Dict* bodies, const char* json);

int load_bodies(Dict* bodies, const char* filename);

void unload_bodies(Dict* bodies);

#endif
