#ifndef LAMBERT_HPP
#define LAMBERT_HPP

#include <glm/glm.hpp>

void lambert(glm::dvec3& v1, glm::dvec3& v2, double mu, const glm::dvec3& r1, const glm::dvec3& r2, double t, int M, int right_branch);

#endif
