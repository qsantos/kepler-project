#include "orbit.hpp"
#include "load.hpp"
#include "recipes.hpp"
#include "lambert.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

// primary                         common primary of origin and target
// injection orbit                 orbit in origin's SoI used to escape
// transfer orbit                  orbit around primary
// insertion orbit                 orbit in target's SoI used to capture
// escape                          when leaving origin SoI
// encounter                       when entering target SoI

double injection_prograde_at_escape_angle(CelestialBody* origin, double r0, double v_soi) {
    /* Consider the injection orbit corresponding to the given velocity vector
     * v_soi and return the angle formed by the position at periapsis and the
     * velocity at escape
     *
     * Parameters:
     *     origin  departed celestial body
     *     r0      periapsis
     *     v_soi   speed at escape
     */
    double mu = origin->gravitational_parameter;
    double r_soi = origin->sphere_of_influence;

    // speed at periapsis
    double v0 = sqrt(v_soi*v_soi + 2.*mu/r0 - 2.*mu/r_soi);

    // true anomaly at escape
    double theta0;
    {
        double e = r0 * v0 * v0 / mu - 1.;  // injection orbit eccentricity
        double a = r0 / (1. - e);  // injection orbit semi-major axis
        theta0 = acos((a*(1.-e*e) - r_soi) / (e*r_soi));
    }

    // zenith angle at escape
    double theta1 = asin(v0*r0 / (v_soi*r_soi));

    return theta0 + theta1;
}

double injection_orbit_inclination_from_vsoi(CelestialBody* origin, double r0, vec3 v_soi) {
    /* Determine the inclination required to reach a specific escape velocity
     *
     * Parameters:
     *     origin  departed celestial body
     *     r0      radius of the parking orbit
     *     v_soi   desired velocity at escape
     */

    // determine the periapsis of the injection orbit by rotating the velocity at escape
    double theta = injection_prograde_at_escape_angle(origin, r0, v_soi.norm());
    // rotate v_soi around z by -theta and project on xy
    double c = cos(-theta);
    double s = sin(-theta);
    vec3 p{
        v_soi[0]*c - v_soi[1]*s,
        v_soi[0]*s + v_soi[1]*c,
        0.,
    };

    // normal of injection orbital plane
    vec3 n = p.cross(v_soi);

    // angle between normals of injection orbital plane and of ecliptic plane
    n /= n.norm();
    return acos(n[2]);
}

double injection_cost(CelestialBody* origin, double parking_radius, vec3 v_escape) {
    /* Return the Δv required to escape from an origin body and reach a specific
     * relative velocity at escape
     */

    // inclination of the in-SoI transfer orbit
    double injection_inclination = injection_orbit_inclination_from_vsoi(origin, parking_radius, v_escape);
    return maneuver_orbit_to_escape_cost(origin, parking_radius, parking_radius, v_escape.norm(), injection_inclination);
}

double insertion_cost(CelestialBody* target, double apsis1, double apsis2, vec3 v_encounter) {
    /* Return the Δv required to insert into an orbit around a target body from
     * a given relative velocity at encounter
     *
     * Parameters:
     *     origin       celestial body to depart
     *     r0           radius of parking orbit
     *     v_encounter  velocity re. target at encounter
     */
    return maneuver_orbit_to_escape_cost(target, apsis1, apsis2, v_encounter.norm(), 0.);
}

double rendez_vous_cost(CelestialBody* origin, CelestialBody* target, double time_at_departure, double transfer_duration, double parking_radius, double apsis1, double apsis2) {
    /* Return the Δv required to transfer from origin to target departing at
     * given time and taking the given time; this assumes a departure from a
     * circular parking orbit at the origin, and an arrival into an elliptical
     * orbit with the given apses at the raget */
    double time_at_arrival = time_at_departure + transfer_duration;

    // state of origin at departure
    vec6 origin_state_at_departure = orbit_state_at_time(origin->orbit, time_at_departure);
    vec3 origin_position_at_departure{origin_state_at_departure[0], origin_state_at_departure[1], origin_state_at_departure[2]};
    vec3 origin_velocity_at_departure{origin_state_at_departure[3], origin_state_at_departure[4], origin_state_at_departure[5]};

    // state of target at arrival
    vec6 target_state_at_arrival = orbit_state_at_time(target->orbit, time_at_arrival);
    vec3 target_position_at_arrival{target_state_at_arrival[0], target_state_at_arrival[1], target_state_at_arrival[2]};
    vec3 target_velocity_at_arrival{target_state_at_arrival[3], target_state_at_arrival[4], target_state_at_arrival[5]};

    // determine transfer orbit
    vec3 transfer_velocity_at_escape, transfer_velocity_at_arrival;
    double mu = origin->orbit->primary->gravitational_parameter;
    lambert(transfer_velocity_at_escape, transfer_velocity_at_arrival, mu, origin_position_at_departure, target_position_at_arrival, transfer_duration, 0, 0);

    // cost of injection into transfer orbit
    vec3 v_escape = transfer_velocity_at_escape - origin_velocity_at_departure;
    double injection_dv = injection_cost(origin, parking_radius, v_escape);

    // cost of insertion into target orbit
    vec3 v_encounter = transfer_velocity_at_arrival - target_velocity_at_arrival;
    double insertion_dv = insertion_cost(target, apsis1, apsis2, v_encounter);

    return injection_dv + insertion_dv;
}

// TODO
static double f(Orbit* trajectory_at_escape, double true_anomaly_at_intercept, double relative_inclination, double x) {
    double plane_change_angle = atan2(tan(relative_inclination), sin(true_anomaly_at_intercept - x));
    double distance = orbit_distance_at_true_anomaly(trajectory_at_escape, x);
    double speed = orbit_speed_at_distance(trajectory_at_escape, distance);
    double dv = maneuver_plane_change_cost(speed, plane_change_angle);
    return dv;
}

double rendez_vous_cost2(CelestialBody* origin, CelestialBody* target, double time_at_departure, double transfer_duration, double parking_radius, double apsis1, double apsis2) {
    double time_at_arrival = time_at_departure + transfer_duration;

    // state of origin at departure
    vec6 origin_state_at_departure = orbit_state_at_time(origin->orbit, time_at_departure);
    vec3 origin_position_at_departure{origin_state_at_departure[0], origin_state_at_departure[1], origin_state_at_departure[2]};
    vec3 origin_velocity_at_departure{origin_state_at_departure[3], origin_state_at_departure[4], origin_state_at_departure[5]};

    // state of target at arrival
    vec6 target_state_at_arrival = orbit_state_at_time(target->orbit, time_at_arrival);
    vec3 target_position_at_arrival{target_state_at_arrival[0], target_state_at_arrival[1], target_state_at_arrival[2]};
    vec3 target_velocity_at_arrival{target_state_at_arrival[3], target_state_at_arrival[4], origin_state_at_departure[5]};

    // determine rotation to bring target on origin's orbital plane
    vec3 n = origin->orbit->orientation * vec3{0, 0, 1};
    // angle between target_position_at_arrival and n
    double relative_inclination = asin(target_position_at_arrival.dot(n) / target_position_at_arrival.norm());
    vec3 rotation_axis = target_position_at_arrival.cross(n);
    mat3 plane_change_rotation = mat3::from_angle_axis(-relative_inclination, rotation_axis[0], rotation_axis[1], rotation_axis[2]);

    // BEGIN BLOCK A
    // use plane_change_rotation to rotate target_position_at_arrival in origin's orbital plane
    vec3 target_position_at_arrival_projected_on_origin_plane = plane_change_rotation * target_position_at_arrival;
    // determine transfer velocities
    vec3 transfer_velocity_at_escape, transfer_velocity_at_arrival;
    double mu = origin->orbit->primary->gravitational_parameter;
    lambert(transfer_velocity_at_escape, transfer_velocity_at_arrival, mu, origin_position_at_departure, target_position_at_arrival_projected_on_origin_plane, transfer_duration, 0, 0);
    // first part of transfer
    Orbit trajectory_at_escape;
    vec6 state{
        origin_position_at_departure[0], origin_position_at_departure[1], origin_position_at_departure[2],
        transfer_velocity_at_escape[0], transfer_velocity_at_escape[1], transfer_velocity_at_escape[2],
    };
    orbit_from_state(&trajectory_at_escape, origin->orbit->primary, state, time_at_departure);
    // find most efficient time to change plane
    double true_anomaly_at_intercept = orbit_true_anomaly_at_distance(&trajectory_at_escape, target_position_at_arrival.norm());  // same as projected
    // Golden Section search
    double a = 0.;  // TODO
    double b = M_PI;  // TODO
    {
#define INV_PHI 0.6180339887498949  // (1. / phi)
#define INV_PHI_2 0.3819660112501051  // (1. / (phi*phi))
        double h = b - a;
        double c = a + INV_PHI_2 * h;
        double d = a + INV_PHI * h;
        double f_c = f(&trajectory_at_escape, true_anomaly_at_intercept, relative_inclination, c);
        double f_d = f(&trajectory_at_escape, true_anomaly_at_intercept, relative_inclination, d);
        for (int i = 0; i < 3; i += 1) {
            if (f_c < f_d) {
                b = d;
                d = c;
                f_d = f_c;
                h *= INV_PHI;
                c = a + INV_PHI_2 * h;
                f_c = f(&trajectory_at_escape, true_anomaly_at_intercept, relative_inclination, c);
            } else {
                a = c;
                c = d;
                f_c = f_d;
                h *= INV_PHI;
                d = a + INV_PHI * h;
                f_d = f(&trajectory_at_escape, true_anomaly_at_intercept, relative_inclination, d);
            }
        }
        if (f_c < f_d) {
            b = d;
        } else {
            a = c;
        }
    }
    (void) b;
    // END BLOCK A


    double plane_change_dv = f(&trajectory_at_escape, true_anomaly_at_intercept, relative_inclination, a);  // TODO
    // TODO: rotate transfer_velocity_at_arrival

    // cost of injection into transfer orbit
    vec3 v_escape = transfer_velocity_at_escape - origin_velocity_at_departure;
    double injection_dv = injection_cost(origin, parking_radius, v_escape);

    // cost of insertion into target orbit
    vec3 v_encounter = transfer_velocity_at_arrival - target_velocity_at_arrival;
    double insertion_dv = insertion_cost(target, apsis1, apsis2, v_encounter);

    return injection_dv + plane_change_dv + insertion_dv;
}

int main(void) {
    /*
    const char* data_file = "data/solar_system.json";
    const char* origin_name = "Earth";
    const char* target_name = "Mars";
    double time_at_departure = 7493. * 86400.;
    double transfer_duration = 180. * 86400.;
    // */

    //*
    const char* data_file = "data/kerbol_system.json";
    const char* origin_name = "Kerbin";
    const char* target_name = "Duna";
    double time_at_departure = 5091552.;
    double transfer_duration = 5588208.;
    // */

    /*
    const char* data_file = "data/kerbol_system.json";
    const char* origin_name = "Kerbin";
    const char* target_name = "Eve";
    double time_at_departure = 12636864.;
    double transfer_duration = 4261464.;
    // */


    Dict bodies;
    if (load_bodies(&bodies, data_file) < 0.) {
        exit(EXIT_FAILURE);
    }
    CelestialBody* origin = bodies.at(origin_name);
    CelestialBody* target = bodies.at(target_name);

    double parking_radius = origin->radius + 100e3;
    double apsis1 = target->radius + 100e3;
    double apsis2 = apsis1;

    /*
    double parking_radius = origin->radius + 200e3;
    double apsis1 = target->radius + 1000e3;
    double apsis2 = target->radius + 3300e3;
    */

    double total_dv = rendez_vous_cost(origin, target, time_at_departure, transfer_duration, parking_radius, apsis1, apsis2);
    printf("%.0f m/s\n", total_dv);
    double total_dv2 = rendez_vous_cost2(origin, target, time_at_departure, transfer_duration, parking_radius, apsis1, apsis2);
    printf("%.0f m/s\n", total_dv2);

    for (size_t i = 0; i < 1<<20; i += 1) {
        rendez_vous_cost2(origin, target, time_at_departure, transfer_duration, parking_radius, apsis1, apsis2);
    }
}
