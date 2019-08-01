#include "vector.hpp"
#include "matrix.hpp"

#include <chrono>
#include <iostream>

struct State : vec6 {
    State(vec6 values) : vec6{values} {}
    State(vec3 position, vec3 velocity) :
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

template<class T>
T runge_kutta_4(T(*f)(double, T), double t, T y, double h) {
    /*
     * Run a numerical integration step `h` on `y` of derivative `f` along `t`
     *
     * Notations from https://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods
     */
    T k1 = f(t, y);
    T k2 = f(t + h / 2., y + k1 * (h / 2.));
    T k3 = f(t + h / 2., y + k2 * (h / 2.));
    T k4 = f(t + h,      y + k3 * h);
    return y + (k1 + (k2 + k3) * 2. + k4) * (h / 6.);
}

vec3 primary{0, 0, 0};

double gravity(double distance) {
    double gravitational_parameter = 398600682732000.;
    return gravitational_parameter / (distance * distance);
}

State f(double t, State state) {
    (void) t;

    vec3 position = {state[0], state[1], state[2]};
    vec3 velocity = {state[3], state[4], state[5]};

    // gravity
    double distance = position.dist(primary);
    double g = gravity(distance);
    vec3 acceleration = position * (-g / distance);

    // propulsion
    //acceleration += thrust

    return {velocity, acceleration};
}

int main(void) {
    State state{
        vec3{6371e3 + 300e3, 0, 0},
        vec3{0, 7660, 0},
    };
    const double dt = 1. / 64;

    // simulate
    auto start = std::chrono::high_resolution_clock::now();
    for (double t = 0; t < 1<<20; t += dt) {
        state = runge_kutta_4(f, t, state, dt);
    }
    std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - start;

    std::cout << state << std::endl;
    std::cout << elapsed.count() << " s" << std::endl;
    return 0;
}
