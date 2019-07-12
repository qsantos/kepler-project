#include "orbit.h"

#include "util.h"

#define TWO_TO_THE_MINUS_26 1.4901161193847656e-08  // 2**-26

int orbit_orientate(Orbit* o, double longitude_of_ascending_node, double inclination, double argument_of_periapsis, double epoch, double mean_anomaly_at_epoch) {
    // normalize inclination within [0, math.pi]
    // a retrograde orbit has an inclination of exactly math.pi
    inclination = fmod2(inclination, 2.*M_PI);
    if (inclination > M_PI) {
        inclination = 2.*M_PI - inclination;
        longitude_of_ascending_node -= M_PI;
        argument_of_periapsis -= M_PI;
    }

    // normalize other angles
    longitude_of_ascending_node = fmod2(longitude_of_ascending_node, 2.*M_PI);
    argument_of_periapsis = fmod2(argument_of_periapsis, 2.*M_PI);

    o->inclination = inclination;
    o->longitude_of_ascending_node = longitude_of_ascending_node;
    o->argument_of_periapsis = argument_of_periapsis;
    o->epoch = epoch;
    o->mean_anomaly_at_epoch = mean_anomaly_at_epoch;

    mat3_from_euler_angles(o->orientation, longitude_of_ascending_node, inclination, argument_of_periapsis);
    return 0;

}
int orbit_from_periapsis(Orbit* o, CelestialBody* primary, double periapsis, double eccentricity) {
    if (eccentricity < 0.) {
        return -1;  // eccentricity must be positive TODO
    }

    o->primary = primary;
    o->periapsis = periapsis;
    o->eccentricity = eccentricity;

    // semi-major axis
    if (eccentricity == 1.) {  // parabolic trajectory
        o->semi_major_axis = INFINITY;
    } else {
        o->semi_major_axis = periapsis / (1. - eccentricity);
    }

    // other distances
    o->apoapsis = o->semi_major_axis * (1. + eccentricity);
    o->semi_latus_rectum = periapsis * (1. + eccentricity);
    double e2 = 1. - eccentricity*eccentricity;
    o->semi_minor_axis = o->semi_major_axis * sqrt(fabs(e2));
    o->focus = o->semi_major_axis * eccentricity;

    // mean motion
    double mu = primary->gravitational_parameter;
    if (eccentricity == 1.) {  // parabolic trajectory
        double l = o->semi_latus_rectum;
        o->mean_motion = 3. * sqrt(mu / (l*l*l));
    } else {
        double a = o->semi_major_axis;
        o->mean_motion = sqrt(mu / fabs(a*a*a));
    }

    // period
    if (eccentricity >= 1.) {  // open trajectory
        o->period = INFINITY;
    } else {  // circular/elliptic orbit
        o->period = 2.*M_PI / o->mean_motion;
    }

    return 0;
}

int orbit_from_semi_major(Orbit* o, CelestialBody* primary, double semi_major_axis, double eccentricity) {
    // check consistency
    if (eccentricity < 1. && semi_major_axis <= 0.) {
        return -1;  // eccentricity < 1. but semi-major axis <= 0. TODO
    }
    if (eccentricity > 1. && semi_major_axis >= 0.) {
        return -1;  // eccentricity > 1. but semi-major axis >= 0. TODO
    }
    if (eccentricity == 1.) {
        return -1;  // cannot define parabolic trajectory from semi-major axis TODO
    }

    // determine periapsis
    double periapsis = semi_major_axis * (1. - eccentricity);

    // define orbit from canonical elements
    return orbit_from_periapsis(o, primary, periapsis, eccentricity);
}

int orbit_from_apses(Orbit* o, CelestialBody* primary, double apsis1, double apsis2) {
    // determine periapsis
    double periapsis = fmin(fabs(apsis1), fabs(apsis2));

    // determine eccentricity
    double eccentricity;
    if (isinf(apsis1) || isinf(apsis2)) {  // parabolic trajectory
        eccentricity = 1.;
    } else {
        eccentricity = fabs(apsis1 - apsis2) / fabs(apsis1 + apsis2);
    }

    // define orbit from canonical elements
    return orbit_from_periapsis(o, primary, periapsis, eccentricity);
}

int orbit_from_period(Orbit* o, CelestialBody* primary, double period, double eccentricity) {
    // check consistency
    if (eccentricity >= 1.) {
        return -1;  // cannot define parabolic/hyperbolic trajectory from period TODO
    }

    // determine semi-major axis
    double mu = primary->gravitational_parameter;
    double mean_motion = period / (2.*M_PI);
    double semi_major_axis = cbrt(mean_motion*mean_motion * mu);

    // define orbit from semi-major axis and eccentricity
    return orbit_from_semi_major(o, primary, semi_major_axis, eccentricity);
}

int orbit_from_period2(Orbit* o, CelestialBody* primary, double period, double apsis) {
    // check consistency
    if (isinf(period)) {
        return -1;  // cannot define parabolic/hyperbolic trajectory from period TODO
    }

    // determine semi-major axis
    double mu = primary->gravitational_parameter;
    double mean_motion = period / (2.*M_PI);
    double semi_major_axis = cbrt(mean_motion*mean_motion * mu);

    // determine eccentricity
    double eccentricity = fabs(apsis / semi_major_axis - 1.);

    // define orbit from semi-major axis and eccentricity
    return orbit_from_semi_major(o, primary, semi_major_axis, eccentricity);
}

int orbit_from_state(Orbit* o, CelestialBody* primary, Vec3 position, Vec3 velocity, double epoch) {
    double mu = primary->gravitational_parameter;

    double distance = vec3_norm(position);
    double speed = vec3_norm(velocity);

    Vec3 x_axis = {1., 0., 0.};
    Vec3 z_axis = {0., 0., 1.};

    Vec3 orbital_plane_normal_vector;
    vec3_cross(orbital_plane_normal_vector, position, velocity);

    // eccentricity
    double pos_factor = speed*speed/mu - 1 / distance;  // v^2/mu - 1/r
    double vel_factor = vec3_dot(position, velocity) / mu;  // r.v / mu
    double eccentricity_vector[3];
    for (int i = 0; i < 3; i++) {
        eccentricity_vector[i] = pos_factor*position[i] - vel_factor*velocity[i];
    }
    double eccentricity = vec3_norm(eccentricity_vector);

    // periapsis
    // from r(t) = 1. / mu * h / (1. + e cos t)
    double specific_angular_momentum = vec3_norm(orbital_plane_normal_vector);
    double periapsis = specific_angular_momentum*specific_angular_momentum / mu / (1. + eccentricity);

    // we have enough information do determine the shape of the orbit
    orbit_from_periapsis(o, primary, periapsis, eccentricity);

    // inclination
    double inclination = vec3_angle(orbital_plane_normal_vector, z_axis);

    // direction of the ascending node
    Vec3 ascend_node_dir = {1., 0., 0.};
    if (inclination != 0. && inclination != M_PI) {
        vec3_cross(ascend_node_dir, z_axis, orbital_plane_normal_vector);
    }

    // longitude of ascending node
    double longitude_of_ascending_node = vec3_angle(x_axis, ascend_node_dir);
    if (orbital_plane_normal_vector[0] < 0.) {
        longitude_of_ascending_node = - longitude_of_ascending_node;
    }

    // argument of periapsis
    double* periapsis_dir = eccentricity != 0. ? eccentricity_vector : x_axis;
    double argument_of_periapsis = vec3_angle2(ascend_node_dir, periapsis_dir, orbital_plane_normal_vector);

    // mean anomaly at epoch
    double true_anomaly_at_epoch = vec3_angle2(periapsis_dir, position, orbital_plane_normal_vector);
    double eccentric_anomaly_at_epoch = orbit_eccentric_anomaly_at_true_anomaly(o, true_anomaly_at_epoch);
    double mean_anomaly_at_epoch = orbit_mean_anomaly_at_eccentric_anomaly(o, eccentric_anomaly_at_epoch);

    // we can now orient the orbit
    orbit_orientate(o, longitude_of_ascending_node, inclination, argument_of_periapsis, epoch, mean_anomaly_at_epoch);
    return 0;
}

double orbit_mean_anomaly_at_time(Orbit* o, double time) {
    return o->mean_anomaly_at_epoch + o->mean_motion * (time - o->epoch);
}

double orbit_eccentric_anomaly_at_mean_anomaly(Orbit* o, double mean_anomaly) {
    double M = mean_anomaly;
    double e = o->eccentricity;

    if (e < 1.) {  // M = E - e sin E
        M = fmod2(M, 2.*M_PI);

        // sin(E) = E -> M = (1. - e) E
        if (fabs(M) < TWO_TO_THE_MINUS_26) {
            return M / (1. - e);
        }

        // Newton's method
        double E = M_PI;
        double previous_E = 0.;
        for (int i = 0; i < 30; i++) {
            double previous_previous_E = previous_E;
            previous_E = E;
            double f_E = E - e*sin(E) - M;  // f(E)
            double fprime_E = 1. - e*cos(E);  // f'(E)
            E -= f_E / fprime_E;
            // exit early if lowest precision is reached
            if (E == previous_E || E == previous_previous_E) {
                break;
            }
        }

        return E;
    } else if (e == 1.) {
        double z = cbrt(M + sqrt(1. + M*M));
        return z - 1./z;
    } else {  // M = e sinh E - E
        // sinh(E) = E -> M = (e - 1.) E
        if (fabs(M) < TWO_TO_THE_MINUS_26) {
            return M / (e - 1.);
        }

        // Newton's method
        double E = 1.;
        double previous_E = 0.;
        for (int i = 0; i < 30; i++) {
            double previous_previous_E = previous_E;
            previous_E = E;
            double f_E = e*sinh(E) - E - M;  // f(E)
            double fprime_E = e*cosh(E) - 1.;  // f'(E)
            E -= f_E / fprime_E;
            // exit early if lowest precision is reached
            if (E == previous_E || E == previous_previous_E) {
                break;
            }
        }

        return E;
    }
}

double orbit_true_anomaly_at_eccentric_anomaly(Orbit* o, double eccentric_anomaly) {
    double E = eccentric_anomaly;
    double e = o->eccentricity;
    if (e < 1.) {  // closed orbit
        double x = sqrt(1.-e) * cos(E/2.);
        double y = sqrt(1.+e) * sin(E/2.);
        return 2. * atan2(y, x);
    } else if (e == 1.) {  // parabolic trajectory
        return 2. * atan(E);
    } else {  // hyperbolic trajectory
        double x = sqrt(e-1.) * cosh(E/2.);
        double y = sqrt(e+1.) * sinh(E/2.);
        return 2. * atan2(y, x);
    }
}

double orbit_eccentric_anomaly_at_true_anomaly(Orbit* o, double true_anomaly) {
    double f = true_anomaly;
    double e = o->eccentricity;
    if (e < 1.) {  // closed orbit
        return 2. * atan(sqrt((1.-e)/(1.+e)) * tan_(f/2.));
    } else if (e == 1.) {  // parabolic trajectory
        return tan_(f / 2.);
    } else {  // hyperbolic trajectory
        return 2. * atanh(sqrt((e-1.)/(e+1.)) * tan_(f/2.));
        /* TODO: might handle some edge case
        double r = sqrt((e-1.)/(e+1.)) * tan_(f/2.);
        if (fabs(ratio) <= 1.) {
            return 2. * atanh(r);
        } else {
            return copysign(INFINITY, r);
        }
        */
    }
}

double orbit_mean_anomaly_at_eccentric_anomaly(Orbit* o, double eccentric_anomaly) {
    double e = o->eccentricity;
    double E = eccentric_anomaly;
    if (e < 1.) {
        return E - e*sin(E);
    } else if (e == 1.) {
        return (E*E*E + E*3.) / 2.;
    } else {
        return e*sinh(E) - E;
    }
}

double orbit_time_at_mean_anomaly(Orbit* o, double mean_anomaly) {
    return o->epoch + (mean_anomaly - o->mean_anomaly_at_epoch) / o->mean_motion;
}

double orbit_distance_at_true_anomaly(Orbit* o, double true_anomaly) {
    return o->semi_latus_rectum / (1. + o->eccentricity*cos(true_anomaly));
}

double orbit_true_anomaly_at_distance(Orbit* o, double distance) {
    // circular orbit
    if (o->eccentricity == 0.) {
        return NAN;
    }

    // periapsis too high
    if (distance < o->periapsis) {
        return NAN;
    }

    // closed orbit and apoapsis too low
    if (o->eccentricity < 1. && o->apoapsis < distance) {
        return NAN;
    }

    // due to rounding errors, the inner part may exceed 1. when distance is
    // close to the periapsis; to avoid problems, we just clamp the value
    return acos(fmin(1., (o->semi_latus_rectum / distance - 1.) / o->eccentricity));
}

double orbit_speed_at_distance(Orbit* o, double distance) {
    double mu = o->primary->gravitational_parameter;
    return sqrt(mu * (2./distance - 1./o->semi_major_axis));
}

void orbit_position_at_true_anomaly(Vec3 res, Orbit* o, double true_anomaly) {
    double distance = orbit_distance_at_true_anomaly(o, true_anomaly);
    double c = cos(true_anomaly);
    double s = sin(true_anomaly);
    Vec3 position = {distance*c, distance*s, 0.};
    mat3_mulv(res, o->orientation, position);
}

void orbit_velocity_at_true_anomaly(Vec3 res, Orbit* o, double true_anomaly) {
    double distance = orbit_distance_at_true_anomaly(o, true_anomaly);
    double c = cos(true_anomaly);
    double s = sin(true_anomaly);
    double e = o->eccentricity;

    double d = 1. + e*c;
    double x = o->semi_latus_rectum*e*s/(d*d);
    Vec3 velocity_direction = {-distance*s + x*c, distance*c + x*s, 0.};

    double norm_v = vec3_norm(velocity_direction);
    double speed = orbit_speed_at_distance(o, distance);

    Vec3 velocity;
    vec3_scale(velocity, velocity_direction, speed/norm_v);
    mat3_mulv(res, o->orientation, velocity);
}

void orbit_position_at_time(Vec3 res, Orbit* o, double time) {
    double M = orbit_mean_anomaly_at_time(o, time);
    double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
    double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
    orbit_position_at_true_anomaly(res, o, f);
}

void orbit_velocity_at_time(Vec3 res, Orbit* o, double time) {
    double M = orbit_mean_anomaly_at_time(o, time);
    double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
    double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
    orbit_velocity_at_true_anomaly(res, o, f);
}

void orbit_state_at_time(Vec3 pos, Vec3 vel, Orbit* o, double time) {
    double M = orbit_mean_anomaly_at_time(o, time);
    double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
    double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
    orbit_position_at_true_anomaly(pos, o, f);
    orbit_velocity_at_true_anomaly(vel, o, f);
}

double orbit_distance_at_time(Orbit* o, double time) {
    double M = orbit_mean_anomaly_at_time(o, time);
    double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
    double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
    return orbit_distance_at_true_anomaly(o, f);
}

double orbit_time_at_distance(Orbit* o, double distance) {
    double f = orbit_true_anomaly_at_distance(o, distance);
    double E = orbit_eccentric_anomaly_at_true_anomaly(o, f);
    double M = orbit_mean_anomaly_at_eccentric_anomaly(o, E);
    return orbit_time_at_mean_anomaly(o, M);
}

double orbit_time_at_escape(Orbit* o) {
    return orbit_time_at_distance(o, o->primary->sphere_of_influence);
}

double orbit_excess_velocity(Orbit* o) {
    if (o->eccentricity < 1.) {
        return NAN;
    }
    double mu = o->primary->gravitational_parameter;
    double a = o->semi_major_axis;
    return sqrt(-mu / a);
}

double orbit_ejection_angle(Orbit* o) {
    // this is equivalent to orbit_true_anomaly_at_distance(INFINITY);
    if (o->eccentricity < 1.) {  // closed orbit
        return NAN;
    } else {
        // when inf = p / (1. + e cos f), we have 1. + e cos f = 0.
        return acos(-1. / o->eccentricity);
    }
}
