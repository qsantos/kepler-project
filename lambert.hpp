#ifndef LAMBERT_HPP
#define LAMBERT_HPP

#include "vector.hpp"

void lambert(vec3& v1, vec3& v2, double mu, const vec3& r1, const vec3& r2, double t, int M, int right_branch);

#endif
