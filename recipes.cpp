#include "recipes.hpp"

extern "C" {
#include "util.h"
}

double darkness_time(Orbit* o) {
    double x = o->primary->radius / o->semi_minor_axis;
    return 2. / o->mean_motion * (asin(x) + o->eccentricity*x);
}

double synodic_period(Orbit* a, Orbit* b) {
    return 1. / fabs(1. / a->period - 1. / b->period);
}

unsigned constellation_minimum_size(CelestialBody* primary, double communication_range) {
    if (communication_range < 0.) {
        return 0;
    }
    return (unsigned) ceil(M_PI / atan(communication_range / primary->radius / 2.));
}

double constellation_minimum_radius(CelestialBody* primary, unsigned size) {
    if (size < 3) {
        return NAN;  // need at least 3 satellites
    }
    return primary->radius / cos(M_PI / size);
}

double constellation_maximum_radius(double communication_range, unsigned size) {
    if (size < 3) {
        return NAN;  // need at least 3 satellites
    }
    return communication_range / sin(M_PI / size) / 2.;
}

double circular_orbit_speed(CelestialBody* primary, double distance) {
    double mu = primary->gravitational_parameter;
    return sqrt(mu / distance);
}

double maneuver_hohmann_cost(CelestialBody* primary, double r1, double r2) {
    double mu = primary->gravitational_parameter;
    double dv1 = sqrt(mu / r1) * (sqrt(2. * r2 / (r1 + r2)) - 1.);
    double dv2 = sqrt(mu / r2) * (1. - sqrt(2. * r1 / (r1 + r2)));
    return dv1 + dv2;
}

double maneuver_hohmann_time(CelestialBody* primary, double r1, double r2) {
    double mu = primary->gravitational_parameter;
    double a = (r1 + r2) / 2.;
    return M_PI * sqrt(a*a*a / mu);
}

double maneuver_bielliptic_cost(CelestialBody* primary, double r1, double r2, double rb) {
    double mu = primary->gravitational_parameter;
    double a1 = (r1 + rb) / 2.;
    double a2 = (r2 + rb) / 2.;
    double dv1 = sqrt(2. * mu / r1 - mu / a1) - sqrt(mu / r1);
    double dv2 = sqrt(2. * mu / rb - mu / a2) - sqrt(2. * mu / rb - mu / a1);
    double dv3 = sqrt(2. * mu / r2 - mu / a2) - sqrt(mu / r2);
    return dv1 + dv2 + dv3;
}

double maneuver_bielliptic_time(CelestialBody* primary, double r1, double r2, double rb) {
    double mu = primary->gravitational_parameter;
    double a1 = (r1 + rb) / 2.;
    double a2 = (r2 + rb) / 2.;
    double t1 = r1 == rb ? 0. : M_PI * sqrt(a1*a1*a1 / mu);
    double t2 = r2 == rb ? 0. : M_PI * sqrt(a2*a2*a2 / mu);
    return t1 + t2;
}

double maneuver_one_tangent_burn_minimum_eccentricity(double r1, double r2) {
    double a = (r1 + r2) / 2.;
    return 1. - r1 / a;
}

double maneuver_one_tangent_burn_time(CelestialBody* primary, double r1, double r2, double e) {
    double a = r1 / (1. - e);
    double f = acos((a*(1.-e*e)/r2-1.) / e);
    // to gain ~10 % in speed, expose raw methods to convert orbit angles
    Orbit orbit;
    orbit_from_periapsis(&orbit, primary, r1, e);
    double E = orbit_eccentric_anomaly_at_true_anomaly(&orbit, f);
    double M = orbit_mean_anomaly_at_eccentric_anomaly(&orbit, E);
    return M / orbit.mean_motion;
}

double maneuver_one_tangent_burn_cost(CelestialBody* primary, double r1, double r2, double e) {
    double mu = primary->gravitational_parameter;
    double a = r1 / (1. - e);
    double dva = sqrt(mu*(2./r1 - 1./a)) - sqrt(mu*(1./r1));
    double f = acos((a*(1.-e*e)/r2-1.) / e);
    double phi = atan(e * sin(f) / (1. + e * cos(f)));
    double vtb = sqrt(mu*(2./r2 - 1./a));
    double vfb = sqrt(mu*(1./r2));
    double dvb = sqrt(vtb*vtb + vfb*vfb - 2.*vtb*vfb * cos(phi));
    return dva + dvb;
}

double maneuver_plane_change_cost(double speed, double angle) {
    return 2. * speed * sin(angle / 2.);
}

double maneuver_inclination_change_cost(Orbit* o, double true_anomaly, double delta_inclination) {
    double e = o->eccentricity;
    double w = o->argument_of_periapsis;
    double f = true_anomaly;
    double n = o->mean_motion;
    double a = o->semi_major_axis;
    double di = delta_inclination;
    return 2. * sin(di/2.) * sqrt(1-e*e) * cos(w+f) * n * a / (1. + e * cos(f));
}

double maneuver_orbit_to_escape_cost(CelestialBody* primary, double r1, double r2, double v_soi, double inclination) {
    double mu = primary->gravitational_parameter;
    double r_soi = primary->sphere_of_influence;

    double a = (r1 + r2) / 2.;
    double r_peri = fmin(r1, r2);
    double v_parking = sqrt(mu * (2./r_peri - 1./a));

    double v_escape = sqrt(v_soi*v_soi + 2*mu*(1./r_peri - 1./r_soi));

    if (inclination == 0.) {
        return v_escape - v_parking;
    } else {
        return sqrt(v_parking*v_parking + v_escape*v_escape - 2.*v_parking*v_escape*cos(inclination));
    }
}
