#ifndef ORBIT_H
#define ORBIT_H

typedef struct orbit Orbit;

#include "body.hpp"

extern "C" {
#include "vector.h"
}

struct orbit {
    // the celestial body being orbited
    CelestialBody* primary;

    // orbital elements
    double periapsis;
    double eccentricity;
    double inclination;
    double longitude_of_ascending_node;
    double argument_of_periapsis;
    double epoch;
    double mean_anomaly_at_epoch;

    // cached useful values
    double semi_major_axis;
    double semi_minor_axis;
    double apoapsis;
    double semi_latus_rectum;
    double focus;
    double mean_motion;
    double period;  // aka. sidereal period

    // cached transform matrix
    Mat3 orientation;  // NOTE: only use after orbit_orientate() has been called
};

// orbit determination
int orbit_orientate      (Orbit* o, double longitude_of_ascending_node, double inclination, double argument_of_periapsis, double epoch, double mean_anomaly_at_epoch);
int orbit_from_periapsis (Orbit* o, CelestialBody* primary, double periapsis, double eccentricity);
int orbit_from_semi_major(Orbit* o, CelestialBody* primary, double semi_major_axis, double eccentricity);
int orbit_from_apses     (Orbit* o, CelestialBody* primary, double apsis1, double apsis2);
int orbit_from_period    (Orbit* o, CelestialBody* primary, double period, double eccentricity);
int orbit_from_period2   (Orbit* o, CelestialBody* primary, double period, double apsis);
int orbit_from_state     (Orbit* o, CelestialBody* primary, Vec3 position, Vec3 velocity, double epoch);

// time/angles conversions
// t -> M -> E -> v
double orbit_mean_anomaly_at_time             (Orbit* o, double time);
double orbit_eccentric_anomaly_at_mean_anomaly(Orbit* o, double mean_anomaly);
double orbit_true_anomaly_at_eccentric_anomaly(Orbit* o, double eccentric_anomaly);
// v -> E -> M -> t
double orbit_eccentric_anomaly_at_true_anomaly(Orbit* o, double true_anomaly);
double orbit_mean_anomaly_at_eccentric_anomaly(Orbit* o, double eccentric_anomaly);
double orbit_time_at_mean_anomaly             (Orbit* o, double mean_anomaly);

// state (distance and speed)
double orbit_distance_at_true_anomaly(Orbit* o, double true_anomaly);
double orbit_true_anomaly_at_distance(Orbit* o, double distance);
double orbit_speed_at_distance       (Orbit* o, double distance);
// state (position and velocity)
// NOTE: only use after orbit_orientate() has been called
void orbit_position_at_true_anomaly(Vec3 res, Orbit* o, double true_anomaly);
void orbit_velocity_at_true_anomaly(Vec3 res, Orbit* o, double true_anomaly);
void orbit_position_at_time        (Vec3 res, Orbit* o, double time);
void orbit_velocity_at_time        (Vec3 res, Orbit* o, double time);
void orbit_state_at_time           (Vec3 pos, Vec3 vel, Orbit* o, double time);

// shortcuts
double orbit_distance_at_time(Orbit* o, double time);
double orbit_time_at_distance(Orbit* o, double distance);
double orbit_time_at_escape  (Orbit* o);  // time at sphere of influence
double orbit_excess_velocity (Orbit* o);  // speed at infinity
double orbit_ejection_angle  (Orbit* o);  // true anomaly at infinity

#endif
