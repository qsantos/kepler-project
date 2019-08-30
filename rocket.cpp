#include "rocket.hpp"

#include <chrono>
#include <iostream>

template<class T>
T euler(T(*f)(double, T), double t, T y, double h) {
    return y + f(t, y) * h;
}

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

// TODO: do not use global variable
static CelestialBody* primary = NULL;
static glm::dvec3 thrust_global;


State f(double t, State state) {
    (void) t;

    glm::dvec3 position = state.position;
    glm::dvec3 velocity = state.velocity;

    // gravity
    glm::dvec3 primary_position{0, 0, 0};  // TODO
    double distance = glm::distance(position, primary_position);
    double g = primary->gravitational_parameter / (distance * distance);
    glm::dvec3 acceleration = position * (-g / distance);

    // propulsion
    acceleration += thrust_global;

    return {velocity, acceleration};
}

void rocket_update(Rocket* rocket, double time, double step, double thrust) {
    primary = rocket->orbit->primary;
    thrust_global = rocket->orientation * glm::dvec3{0, 0, thrust};
    rocket->state = runge_kutta_4(f, time, rocket->state, step);
}
