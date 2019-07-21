#ifndef BODY_H
#define BODY_H

typedef struct celestial_body CelestialBody;

#include "orbit.hpp"

#include <stddef.h>

extern "C" {
#include "vector.h"
#include "coordinates.h"
}

struct celestial_body {
    const char* name;
    double radius;

    // gravity
    double gravitational_parameter;
    double mass;
    size_t n_satellites;
    CelestialBody** satellites;

    // orbit
    Orbit* orbit;
    double sphere_of_influence;

    // rotation
    CelestialCoordinates* north_pole;
    double sidereal_day;
    double synodic_day;  // aka. solar day for Earth
    double tilt;
    double surface_velocity;
    double rotational_speed;

};

void body_init (CelestialBody* body);
void body_clear(CelestialBody* body);

void body_set_name     (CelestialBody* body, const char* name);
void body_set_radius   (CelestialBody* body, double radius);
void body_set_gravparam(CelestialBody* body, double gravitational_parameter);
void body_set_mass     (CelestialBody* body, double mass);
void body_set_orbit    (CelestialBody* body, Orbit* orbit);
void body_set_axis     (CelestialBody* body, CelestialCoordinates* north_pole);
void body_set_rotation (CelestialBody* body, double sidereal_day);

double body_gravity         (CelestialBody* body, double distance);
double body_escape_velocity (CelestialBody* body, double distance);
double body_angular_diameter(CelestialBody* body, double distance);

void body_append_satellite(CelestialBody* body, CelestialBody* satellite);
void body_remove_satellite(CelestialBody* body, CelestialBody* satellite);

void body_global_position_at_time(Vec3 pos, CelestialBody* body, double time);

#endif
