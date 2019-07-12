#ifndef RECIPES_H
#define RECIPES_H

#include "util.h"
#include "body.h"
#include "orbit.h"

double darkness_time(Orbit* o);
double synodic_period(Orbit* a, Orbit* b);

unsigned constellation_minimum_size  (CelestialBody* primary, double communication_range);
double   constellation_minimum_radius(CelestialBody* primary, unsigned size);
double   constellation_maximum_radius(double communication_range, unsigned size);

double circular_orbit_speed(CelestialBody* primary, double distance);

double maneuver_hohmann_cost(CelestialBody* primary, double r1, double r2);
double maneuver_hohmann_time(CelestialBody* primary, double r1, double r2);
double maneuver_bielliptic_cost(CelestialBody* primary, double r1, double r2, double rb);
double maneuver_bielliptic_time(CelestialBody* primary, double r1, double r2, double rb);
double maneuver_one_tangent_burn_minimum_eccentricity(double r1, double r2);
double maneuver_one_tangent_burn_time(CelestialBody* primary, double r1, double r2, double e);
double maneuver_one_tangent_burn_cost(CelestialBody* primary, double r1, double r2, double e);
// TODO: check plane change vs inclination change
double maneuver_plane_change_cost(double speed, double angle);
double maneuver_inclination_change_cost(Orbit* o, double true_anomaly, double delta_inclination);
double maneuver_orbit_to_escape_cost(CelestialBody* primary, double r1, double r2, double v_soi, double inclination);

// NOTE: HOHMANN_WORST_RATIO    applies to R = r2/r1
//       HOHMANN_UPPER_BOUND    applies to R = r2/r1
//       BIELLIPTIC_LOWER_BOUND applies to R*  = rb/r1

// worst r2/r1 ratio in terms of delta v for a Hohmann transfer with r1 fixed
// 5. + 4.*sqrt(7.)*cos(1./3.*atan(sqrt(3.)/37.))
#define HOHMANN_WORST_RATIO 15.581718738763179

// as long as r2/r1 is under this value, the Hohmann transfer is always cheaper
// than any other Bi-elliptic transfer
// most interesting solution to R**3 - (7+4*sqrt(2))*R**2 + (3+4*sqrt(2))*R - 1 = 0
// it is identical to HOHMANN_WORST_RATIO
#define HOHMANN_UPPER_BOUND  11.938765472645871

// this is the smallest rb/r1 where a bi-elliptical transfer *can* be more
// efficient than a Hohmann transfer
// NOTE: that, when 11.94 < r2/r1 < 15.58, a bi-elliptical transfer may still
// be more effective than a Hohmann transfer, but rb/r1 will be at least 15.58
// EXAMPLE: for r2 = 14*r1, the bi-elliptical transfer with rb = 50*r1 is
// cheaper than a Hohmann transfer; however, with rb = 20*r1, it would not be
#define BIELLIPTIC_LOWER_BOUND 15.581718738763179  // most interesting solution to R**3 - 15*R**2 - 9*R - 1 = 0

#endif
