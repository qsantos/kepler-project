#ifndef ROCKET_HPP
#define ROCKET_HPP

#include "orbit.hpp"

#include <glm/glm.hpp>
#include <glm/ext/quaternion_double.hpp>

struct State {
    glm::dvec3 position;
    glm::dvec3 velocity;

    State operator*(double k) {
        State ret{
            position * k,
            velocity * k,
        };
        return ret;
    }

    State operator+(State rhs) {
        State ret{
            position + rhs.position,
            velocity + rhs.velocity,
        };
        return ret;
    }
};

// TODO: Orbiter should be a component of CelestialBody and Rocket
struct Rocket : CelestialBody {
    State state;
    glm::dquat orientation = glm::identity<glm::dquat>();
    glm::dquat angular_velocity = glm::identity<glm::dquat>();
    double throttle = 0.;
};

void rocket_update(Rocket* rocket, double time, double step, double thrust);

#endif
