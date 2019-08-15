#ifndef ROCKET_HPP
#define ROCKET_HPP

#include "vector.hpp"
#include "orbit.hpp"

struct State : vec6 {
    State() {}
    State(const vec6& values) : vec6{values} {}
    State(const vec3& position, const vec3& velocity) :
        vec6{
            position[0], position[1], position[2],
            velocity[0], velocity[1], velocity[2],
        }
    {
    }

    vec3 position() const {
        const State& self = *this;
        return {self[0], self[1], self[2]};
    }

    vec3 velocity() const {
        const State& self = *this;
        return {self[3], self[4], self[5]};
    }
};

// TODO: Orbiter should be a component of CelestialBody and Rocket
struct Rocket : CelestialBody {
    State state;
    double orbit_updated;
};

void rocket_update(Rocket* rocket, double time, double step);

#endif
