#ifndef LAMBERT_HPP
#define LAMBERT_HPP

extern "C" {
#include "vector.h"
}

void lambert(Vec3 v1, Vec3 v2, double mu, Vec3 r1, Vec3 r2, double t, int M, int right_branch);

#endif
