#ifndef BODY_H
#define BODY_H

struct CelestialBody;

#include "coordinates.hpp"
#include "orbit.hpp"

#include <stddef.h>

struct CelestialBody {
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
    CelestialCoordinates* positive_pole;
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
void body_set_axis     (CelestialBody* body, CelestialCoordinates* positive_pole);
void body_set_rotation (CelestialBody* body, double sidereal_day);

double body_gravity         (CelestialBody* body, double distance);
double body_escape_velocity (CelestialBody* body, double distance);
double body_angular_diameter(CelestialBody* body, double distance);

void body_append_satellite(CelestialBody* body, CelestialBody* satellite);
void body_remove_satellite(CelestialBody* body, CelestialBody* satellite);

glm::dvec3 body_global_position_at_time(CelestialBody* body, double time);

#endif
