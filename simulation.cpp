#include "simulation.hpp"

#include "matrix.hpp"

#include <chrono>
#include <iostream>

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

void rocket_update(Rocket* rocket, double time, double step) {
    rocket->state = runge_kutta_4(f, time, rocket->state, step);
}
