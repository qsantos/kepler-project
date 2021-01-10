#include "body.hpp"

extern "C" {
#include "util.h"
#include "logging.h"
}

static const double G = 6.67259e-11;

void body_init(CelestialBody* body) {
    *body = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {}};
}

void body_clear(CelestialBody* body) {
    free(body->satellites);
    free(body->positive_pole);
    free(body->orbit);
}

static void _body_update_sphere_of_influence(CelestialBody* body) {
    // sphere of influence
    if (body->orbit == NULL) {
        body->sphere_of_influence = INFINITY;
    } else {
        double a = body->orbit->semi_major_axis;
        double mu_p = body->orbit->primary->gravitational_parameter;
        double mu_b = body->gravitational_parameter;
        double soi = a * pow(mu_b / mu_p, 0.4);
        body->sphere_of_influence = soi;
    }
}

static void _body_update_tilt(CelestialBody* body) {
    if (body->positive_pole == NULL || body->orbit == NULL) {
        body->tilt = 0.;
        return;
    }

    /* from http://www.krysstal.com/sphertrig.html
     * the blue great circle is the ecliptic
     * A is the normal of the ecliptic
     * B is the north pole of the body
     * C is the normal of the orbital plane
     * a is the axial tilt of the body
     * b is the orbital inclination
     * c is the complement of the ecliptic latitude of the north pole
     * B' is the ecliptic longitude of the north pole
     * C' is orthogonal to the line of nodes
     */
    double b = body->orbit->inclination;
    double c = body->positive_pole->ecliptic_latitude - M_PI/2.;
    if (body->sidereal_day < 0.) {  // retrograde rotation
        c += M_PI;
    }
    double A = body->orbit->longitude_of_ascending_node + M_PI/2. - body->positive_pole->ecliptic_longitude;
    double ca = cos(b)*cos(c) + sin(b)*sin(c)*cos(A);
    body->tilt = acos(ca);
}

static void _body_update_solar_day(CelestialBody* body) {
    if (body->orbit == NULL) {
        body->synodic_day = NAN;
        return;
    }

    double sidereal_day = body->sidereal_day;
    double sidereal_year = body->orbit->period;
    double solar_year = sidereal_year - sidereal_day;
    if (solar_year == 0.) {
        body->synodic_day = INFINITY;
    } else {
        body->synodic_day = sidereal_day * sidereal_year/solar_year;
    }
}

static void _body_update_tidal_locking(CelestialBody* body) {
    if (body->sidereal_day == 0. && body->orbit != NULL) {
        body->sidereal_day = body->orbit->period;
    }
}

static void _body_update_angular_velocity(CelestialBody* body) {
    if (body->angular_speed == 0.) {
        body->angular_velocity = {0., 0., 0.};
        return;
    }
    glm::dvec3 axis = {0., 0., 1.};
    if (body->positive_pole != NULL) {
        double x_angle = body->positive_pole->ecliptic_latitude - M_PI / 2.;
        axis = glm::dmat3(glm::rotate(glm::dmat4(1), x_angle, glm::dvec3(1., 0., 0.))) * axis;
        double z_angle = body->positive_pole->ecliptic_longitude - M_PI / 2.;
        axis = glm::dmat3(glm::rotate(glm::dmat4(1), z_angle, glm::dvec3(0., 0., 1.))) * axis;
    }
    body->angular_velocity = axis * body->angular_speed;
}

void body_set_name(CelestialBody* body, const char* name) {
    body->name = name;
}

void body_set_radius(CelestialBody* body, double radius) {
    body->radius = radius;
}

void body_set_gravparam(CelestialBody* body, double gravitational_parameter) {
    body->mass = gravitational_parameter / G;
    body->gravitational_parameter = gravitational_parameter;
    _body_update_sphere_of_influence(body);
}

void body_set_mass(CelestialBody* body, double mass) {
    body->mass = mass;
    body->gravitational_parameter = G * mass;
    _body_update_sphere_of_influence(body);
}

void body_set_orbit(CelestialBody* body, Orbit* orbit) {
    if (body->orbit != NULL) {
        body_remove_satellite(body->orbit->primary, body);
    }
    body->orbit = orbit;
    if (body->orbit != NULL) {
        body_append_satellite(body->orbit->primary, body);
    }
    _body_update_sphere_of_influence(body);
    _body_update_tidal_locking(body);
    _body_update_tilt(body);
    _body_update_solar_day(body);
}

void body_set_rotation(CelestialBody* body, double sidereal_day) {
    body->sidereal_day = sidereal_day;
    body->angular_speed = 2.*M_PI / sidereal_day;
    _body_update_tidal_locking(body);
    _body_update_tilt(body);
    _body_update_solar_day(body);
    _body_update_angular_velocity(body);
}

void body_set_axis(CelestialBody* body, CelestialCoordinates* positive_pole) {
    body->positive_pole = positive_pole;
    _body_update_tilt(body);
    _body_update_angular_velocity(body);
}

double body_gravity(CelestialBody* body, double distance) {
    double mu = body->gravitational_parameter;
    if (distance == 0.) {
        return 0.;
    } else if (distance < body->radius) {
        // see https://en.wikipedia.org/wiki/Shell_theorem
        mu *= pow(distance / body->radius, 3.);
    }
    return mu / (distance*distance);
}

double body_escape_velocity(CelestialBody* body, double distance) {
    double mu = body->gravitational_parameter;
    // default escape velocity from surface
    if (distance < body->radius) {
        // see https://www.quora.com/What-is-the-escape-velocity-at-the-center-of-the-earth
        double r = distance;
        double R = body->radius;
        return sqrt(mu * (3./R - r*r/(R*R*R)));
    }
    return sqrt(2. * mu / distance);
}

double body_angular_diameter(CelestialBody* body, double distance) {
    return 2. * asin(body->radius / distance);
}

void body_append_satellite(CelestialBody* body, CelestialBody* satellite) {
    body->satellites = (CelestialBody**) REALLOC(body->satellites, sizeof(CelestialBody*) * (body->n_satellites+1));
    body->satellites[body->n_satellites] = satellite;
    body->n_satellites += 1;
}

void body_remove_satellite(CelestialBody* body, CelestialBody* satellite) {
    for (size_t i = 0; i < body->n_satellites; i += 1) {
        if (body->satellites[i] == satellite) {
            body->n_satellites -= 1;
            body->satellites[i] = body->satellites[body->n_satellites];
            break;
        }
    }
}

glm::dvec3 body_global_position_at_time(CelestialBody* body, double time) {
    if (body->orbit == NULL) {
        return {0, 0, 0};
    }
    if (body->orbit->primary == body) {
        CRITICAL("'%s' is its own primary", body->name);
        exit(EXIT_FAILURE);
    }
    glm::dvec3 primary_position = body_global_position_at_time(body->orbit->primary, time);
    glm::dvec3 relative_position = orbit_position_at_time(body->orbit, time);
    return primary_position + relative_position;
}
