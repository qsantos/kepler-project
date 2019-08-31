#include "body.hpp"
#include "orbit.hpp"
#include "load.hpp"
#include "coordinates.hpp"
#include "recipes.hpp"
#include "lambert.hpp"
#include "rocket.hpp"

extern "C" {
#include "util.h"
}

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define countof(A) (sizeof(A)/sizeof((A)[0]))

#define d 1e-7
static const double angle_testset[] = {
    -M_PI, -M_PI+d,
    -M_PI/2.-d, -M_PI/2., -M_PI/2.+d,
    -M_PI/4.-d, -M_PI/4., -M_PI/4.+d,
    -d, 0., d,
    M_PI/4.-d, M_PI/4., M_PI/4.+d,
    M_PI/2.-d, M_PI/2., M_PI/2.+d,
    M_PI-d, M_PI};
#undef d

static void _assert(int ret, const char* filename, int line, const char* funcname, const char* code);
static void _assertFails(int ret, const char* filename, int line, const char* funcname, const char* code);
static void _assertEquals(double a, double b, const char* filename, int line, const char* funcname, const char* codea, const char* codeb);
static void _assertIsClose(double a, double b, const char* filename, int line, const char* funcname, const char* codea, const char* codeb);
static void _assertIsCloseAngle(double a, double b, const char* filename, int line, const char* funcname, const char* codea, const char* codeb);

#undef assert
#define assert(ret) _assert(ret, __FILE__, __LINE__, __func__, #ret)
#define assertFails(ret) _assertFails(ret, __FILE__, __LINE__, __func__, #ret)
#define assertEquals(a, b) _assertEquals(a, b, __FILE__, __LINE__, __func__, #a, #b)
#define assertIsLower(a, b) _assertIsLower(a, b, __FILE__, __LINE__, __func__, #a, #b)
#define assertIsClose(a, b) _assertIsClose(a, b, __FILE__, __LINE__, __func__, #a, #b)
#define assertIsCloseAngle(a, b)  _assertIsCloseAngle(a, b, __FILE__, __LINE__, __func__, #a, #b)

static void _assert(int ret, const char* filename, int line, const char* funcname, const char* code) {
    if (ret) {
        return;
    }
    fprintf(stderr, "FAILED (%s:%i in %s) `%s` (false)\n", filename, line, funcname, code);
}

static void _assertFails(int ret, const char* filename, int line, const char* funcname, const char* code) {
    if (ret < 0) {
        return;
    }
    fprintf(stderr, "FAILED (%s:%i in %s) `%s` (returned %i)\n", filename, line, funcname, code, ret);
}

static void _assertEquals(double a, double b, const char* filename, int line, const char* funcname, const char* codea, const char* codeb) {
    if ((isnan(a) && isnan(b)) || a == b) {
        return;
    }
    fprintf(stderr, "FAILED (%s:%i in %s) `%s == %s` (%.17g != %.17g)\n", filename, line, funcname, codea, codeb, a, b);
}

static void _assertIsLower(double a, double b, const char* filename, int line, const char* funcname, const char* codea, const char* codeb) {
    if ((isnan(a) && isnan(b)) || a < b) {
        return;
    }
    fprintf(stderr, "FAILED (%s:%i in %s) `%s < %s` (%.17g >= %.17g)\n", filename, line, funcname, codea, codeb, a, b);
}

static void _assertIsClose(double a, double b, const char* filename, int line, const char* funcname, const char* codea, const char* codeb) {
    if ((isnan(a) && isnan(b)) || isclose(a, b)) {
        return;
    }
    fprintf(stderr, "FAILED (%s:%i in %s) `%s ~ %s` (%.17g != %.17g)\n", filename, line, funcname, codea, codeb, a, b);
}

static void _assertIsCloseAngle(double a, double b, const char* filename, int line, const char* funcname, const char* codea, const char* codeb) {
    if ((isnan(a) && isnan(b)) || fmod2(b-a + M_PI, 2.*M_PI) - M_PI < 1e-7) {
        return;
    }
    fprintf(stderr, "FAILED (%s:%i in %s) `%s ~ %s` (angle %.17g != %.17g)\n", filename, line, funcname, codea, codeb, a, b);
}

static void assertIsCloseOrbit(Orbit* a, Orbit* b) {
    assertIsClose(a->periapsis, b->periapsis);
    assertIsClose(a->eccentricity, b->eccentricity);
    assertIsCloseAngle(a->inclination, b->inclination);

    // longitude of ascending node
    if (a->inclination != 0. && a->inclination != M_PI) {  // gimpbal lock
        assertIsCloseAngle(a->longitude_of_ascending_node, b->longitude_of_ascending_node);
    }

    if (a->eccentricity != 0.) {
        // argument of periapsis
        // (not well defined in circular orbits)
        double argument_of_periapsis_a = a->argument_of_periapsis;
        double argument_of_periapsis_b = b->argument_of_periapsis;
        // when inclination is 0. or M_PI, argument of periapsis and longitude
        // of ascending node must be merged into a single value (gimbal
        // lock), normally called 'longitude of periapsis', since this is a
        // special case, we will keep the name 'argument of periapsis' here
        if (a->inclination == 0.) {
            argument_of_periapsis_a += a->longitude_of_ascending_node;
            argument_of_periapsis_b += b->longitude_of_ascending_node;
        }
        if (a->inclination == M_PI) {
            argument_of_periapsis_a -= a->longitude_of_ascending_node;
            argument_of_periapsis_b -= b->longitude_of_ascending_node;
        }
        assertIsCloseAngle(argument_of_periapsis_a, argument_of_periapsis_b);

        // mean anomaly
        // (not well defined in circular orbits)
        double mean_anomaly_a = orbit_mean_anomaly_at_time(a, 0.);
        double mean_anomaly_b = orbit_mean_anomaly_at_time(b, 0.);
        assertIsCloseAngle(mean_anomaly_a, mean_anomaly_b);
    }
}

CelestialBody make_dummy_object(double radius, double gravitational_parameter, double sphere_of_influence) {
    return {
        NULL,  // name
        radius,  // radius
        gravitational_parameter, // gravitational_parameter
        0,  // mass
        0,  // n_satellites
        NULL,  // satellites
        NULL,  // orbit
        sphere_of_influence,  // sphere_of_influence
        NULL, // north_pole
        0,  // sidereal_day
        0,  // synodic_day
        0,  // tilt
        0,  // surface_velocity
        0,  // rotational_speed
    };
}

CelestialBody primary = make_dummy_object(0, 1e20, 1e9);

Orbit make_dummy_orbit_with_period(double period) {
    return {
        NULL,  // primary
        0,  // periapsis
        0,  // eccentricity
        0,  // inclination
        0,  // longitude_of_ascending_node
        0,  // argument_of_periapsis
        0,  // epoch
        0,  // mean_anomaly_at_epoch
        0,  // semi_major_axis
        0,  // semi_minor_axis
        0,  // apoapsis
        0,  // semi_latus_rectum
        0,  // focus
        0,  // mean_motion
        period,  // period
        glm::identity<glm::dquat>(),  // orientation
    };
}

static void test_coordinates(void) {

    // use known values and check consistency
    {
        // coordinates of Polaris (Alpha Ursae Minoris Aa)
        double right_ascension = (2.*3600.+31.*60.+49.9) / 86400. * 2.*M_PI;
        double declination = (89. + 15./60. + 50.8/3600.) / 360. * 2.*M_PI;
        double distance = 433. * 86400. * 365.25 * 299792458.;
        // close enough to the output of https://ned.ipac.caltech.edu/forms/calculator.html
        auto c = CelestialCoordinates::from_equatorial(right_ascension, declination, distance);
        assertIsCloseAngle(c.ecliptic_longitude, 1.5375118630442743);
        assertIsCloseAngle(c.ecliptic_latitude, 1.15090057073079);
        // convert back and check consistency
        c = CelestialCoordinates::from_ecliptic(c.ecliptic_longitude, c.ecliptic_latitude, c.distance);
        assertIsCloseAngle(c.right_ascension, right_ascension);
        assertIsCloseAngle(c.declination, declination);
    }

    // only check consistency for arbitrary values
    for (size_t i = 0; i < countof(angle_testset); i += 1) {
        for (size_t j = 0; j < countof(angle_testset); j += 1) {
            {
                double right_ascension = angle_testset[i];
                double declination = angle_testset[j] / 2.;
                //fprintf(stderr, "α = %f δ = %f\n", right_ascension, declination);
                auto c = CelestialCoordinates::from_equatorial(right_ascension, declination, 0.);
                c = CelestialCoordinates::from_ecliptic(c.ecliptic_longitude, c.ecliptic_latitude, 0.);
                if (!isclose(fmod2(declination, M_PI), M_PI/2.)) {
                    assertIsCloseAngle(c.right_ascension, right_ascension);
                }
                assertIsCloseAngle(c.declination, declination);
            }
            // do it the other way too
            {
                double ecliptic_longitude = angle_testset[i];
                double ecliptic_latitude = angle_testset[j] / 2.;
                //fprintf(stderr, "λ = %f β = %f\n", ecliptic_longitude, ecliptic_latitude);
                auto c = CelestialCoordinates::from_ecliptic(ecliptic_longitude, ecliptic_latitude, 0.);
                c = CelestialCoordinates::from_equatorial(c.right_ascension, c.declination, 0.);
                if (!isclose(fmod2(ecliptic_latitude, M_PI), M_PI/2.)) {
                    assertIsCloseAngle(c.ecliptic_longitude, ecliptic_longitude);
                }
                assertIsCloseAngle(c.ecliptic_latitude, ecliptic_latitude);
            }
        }
    }
}

static void test_body(void) {
    CelestialBody b;
    body_init(&b);

    // before setting a primary, there should be no concept of solar days
    body_set_rotation(&b, 0.);
    assertEquals(b.synodic_day, NAN);

    // body_set_name() may copy the string but the contents should be the same
    const char* name = "Vénus (金星)";
    body_set_name(&b, name);
    assert(strcmp(b.name, name) == 0);

    // really not much room for error
    double radius = 1e8 + 1e-8;
    body_set_radius(&b, radius);
    assertEquals(b.radius, radius);

    // still quite simple
    double gravitational_parameter = 1e12;
    body_set_gravparam(&b, gravitational_parameter);
    body_set_mass(&b, b.mass);
    assertEquals(b.mass, 1.4986684330971933e+22);
    assertEquals(b.gravitational_parameter, gravitational_parameter);

    {
        // again, body_set_orbit() may copy its parameter
        Orbit o;
        // Venus orbit
        orbit_from_periapsis(&o, &primary, 107.477e6, 0.006772);
        orbit_orientate(&o, radians(76.680), radians(3.39458), degrees(54.884), 0., 0.);
        body_set_orbit(&b, &o);
        assertIsCloseOrbit(b.orbit, &o);

        // rotational speed
        body_set_rotation(&b, 0.);
        assertEquals(b.sidereal_day, o.period);
        assertEquals(b.synodic_day, INFINITY);
        body_set_rotation(&b, 42.41);
        assertIsClose(b.rotational_speed * b.sidereal_day, 2.*M_PI);
        assertIsClose(2*M_PI*b.radius / b.surface_velocity, b.sidereal_day);
    }

    CelestialCoordinates positive_pole;
    {
        // celestial coordinates might just be copied as well
        // Venus north pole
        double right_ascension = radians(272.76);
        double declination = radians(67.16);
        positive_pole = CelestialCoordinates::from_equatorial(right_ascension, declination, INFINITY);
        body_set_axis(&b, &positive_pole);
        assertIsCloseAngle(b.tilt, radians(2.6378801547605204));
        // a negative rotational period should invert the tilt
        body_set_rotation(&b, -42.41);
        assertIsCloseAngle(b.tilt, radians(177.36211984523948));
    }

    // gravity
    // edge cases
    assertEquals(body_gravity(&b, 0.), 0.);
    assertEquals(body_gravity(&b, INFINITY), 0.);
    // sign of slope
    assertIsLower(body_gravity(&b, b.radius - .1), body_gravity(&b, b.radius));
    assertIsLower(body_gravity(&b, b.radius + .1), body_gravity(&b, b.radius));
    // continuity
    assertIsClose(body_gravity(&b, b.radius - 1e-5), body_gravity(&b, b.radius));
    assertIsClose(body_gravity(&b, b.radius + 1e-5), body_gravity(&b, b.radius));

    // escape velocity
    // edge cases
    assertIsClose(body_escape_velocity(&b, 0.), sqrt(1.5) * body_escape_velocity(&b, b.radius));
    assertEquals(body_escape_velocity(&b, INFINITY), 0.);
    // sign of slope
    assertIsLower(body_escape_velocity(&b, b.radius), body_escape_velocity(&b, b.radius - .1));
    assertIsLower(body_escape_velocity(&b, b.radius + .1), body_escape_velocity(&b, b.radius));
    // continuity
    assertIsClose(body_escape_velocity(&b, b.radius), body_escape_velocity(&b, b.radius - 1e-6));
    assertIsClose(body_escape_velocity(&b, b.radius + 1e-6), body_escape_velocity(&b, b.radius));

    // angular diameter
    assertEquals(body_angular_diameter(&b, 0.), NAN);
    assertEquals(body_angular_diameter(&b, b.radius), M_PI);
    assertEquals(body_angular_diameter(&b, INFINITY), 0.);
    assertIsLower(body_angular_diameter(&b, b.radius*10.), body_angular_diameter(&b, b.radius));

    // global position
    {
        // now, we can check that the body is actually moving
        glm::dvec3 pos0 = body_global_position_at_time(&b, 0.);
        glm::dvec3 pos1 = body_global_position_at_time(&b, 1.);
        assert(glm::distance(pos0, pos1) != 0.);
    }

    // satellite management
    {
        CelestialBody s1, s2;
        body_init(&s1);
        body_init(&s2);
        Orbit o;
        orbit_from_periapsis(&o, &b, 100e3, 0.);
        body_set_orbit(&s1, &o);
        assert(b.n_satellites == 1);
        body_append_satellite(&b, &s2);
        assert(b.n_satellites == 2);
        body_set_orbit(&s1, NULL);
        assert(b.n_satellites == 1);
        body_remove_satellite(&b, &s2);
        assert(b.n_satellites == 0);
    }
}

static void test_orbit(Orbit* o) {
    // check true anomaly at periapsis and apoapsis
    {
        double M = orbit_mean_anomaly_at_time(o, 0.);
        double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
        double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
        assertIsCloseAngle(M, 0.);
        assertIsCloseAngle(E, 0.);
        assertIsCloseAngle(f, 0.);
    }
    if (o->eccentricity < 1.) {  // only closed orbits have apoapses
        double apoapsis_time = (M_PI - o->mean_anomaly_at_epoch) / o->mean_motion;
        double M = orbit_mean_anomaly_at_time(o, apoapsis_time);
        double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
        double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
        assertIsCloseAngle(M, M_PI);
        assertIsCloseAngle(E, M_PI);
        assertIsCloseAngle(f, M_PI);
    }

    // set orientation of re-generated orbit; only touched by orbit_from_state()
    Orbit _p;
    Orbit* p = &_p;
    orbit_orientate(p, o->longitude_of_ascending_node, o->inclination, o->argument_of_periapsis, 0., 0.);

    // re-generate from semi-major axis
    if (o->eccentricity != 1.) {  // parabolic trajectories have infinite semi-major axis
        orbit_from_semi_major(p, o->primary, o->semi_major_axis, o->eccentricity);
        assertIsCloseOrbit(o, p);
    }

    // re-generate from apses
    orbit_from_apses(p, o->primary, o->periapsis, o->apoapsis);
    assertIsCloseOrbit(o, p);
    // also try with inverted apses
    orbit_from_apses(p, o->primary, o->apoapsis, o->periapsis);
    assertIsCloseOrbit(o, p);

    // re-generate from orbital period ...
    if (o->eccentricity < 1.) {  // open trajectories have no period
        // ... and eccentricity
        orbit_from_period(p, o->primary, o->period, o->eccentricity);
        assertIsCloseOrbit(o, p);

        // ... and periapsis
        orbit_from_period2(p, o->primary, o->period, o->periapsis);
        assertIsCloseOrbit(o, p);

        // ... and apoapsis
        orbit_from_period2(p, o->primary, o->period, o->apoapsis);
        assertIsCloseOrbit(o, p);
    }

    // re-generate from state point at arbitrary time
    {
        double time = 1e4;
        glm::dvec3 position = orbit_position_at_time(o, time);
        glm::dvec3 velocity = orbit_velocity_at_time(o, time);
        orbit_from_state(p, o->primary, position, velocity, time);
        assertIsCloseOrbit(o, p);

        // since we have the position and velocity vectors, we can also check
        // orbit_distance_at_*() and orbit_speed_at_*()
        double M = orbit_mean_anomaly_at_time(o, time);
        double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
        double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
        double distance = glm::length(position);
        double speed = glm::length(velocity);
        assertIsClose(orbit_distance_at_time(o, time), distance);
        assertIsClose(orbit_distance_at_true_anomaly(o, f), distance);
        assertIsClose(orbit_speed_at_distance(o, distance), speed);
    }

    // check conversions of anomalies
    {
        double time = 1e4;
        double M = orbit_mean_anomaly_at_time(o, time);
        double E = orbit_eccentric_anomaly_at_mean_anomaly(o, M);
        double f = orbit_true_anomaly_at_eccentric_anomaly(o, E);
        assertIsCloseAngle(orbit_eccentric_anomaly_at_mean_anomaly(o, M), E);
        assertIsCloseAngle(orbit_true_anomaly_at_eccentric_anomaly(o, E), f);
        assertIsCloseAngle(orbit_eccentric_anomaly_at_true_anomaly(o, f), E);
        assertIsCloseAngle(orbit_mean_anomaly_at_eccentric_anomaly(o, E), M);

        // check that the mean anomaly is linear with time
        assertIsClose(orbit_time_at_mean_anomaly(o, M+M_PI/8.), time + o->period / 16.);
        assertIsClose(orbit_time_at_mean_anomaly(o, M+M_PI/4.), time + o->period / 8.);
        assertIsClose(orbit_time_at_mean_anomaly(o, M+M_PI/2.), time + o->period / 4.);;
        assertIsClose(orbit_time_at_mean_anomaly(o, M+M_PI), time + o->period / 2.);
        assertIsClose(orbit_time_at_mean_anomaly(o, M+2.*M_PI), time + o->period);
    }

    // hyperbolic excess velocity and ejection angle
    {
        assertIsClose(orbit_excess_velocity(o), orbit_speed_at_distance(o, INFINITY));
        assertIsClose(orbit_ejection_angle(o), orbit_true_anomaly_at_distance(o, INFINITY));
    }

    // time at distance
    if (o->eccentricity > 0.) {  // non-circular orbit
        // a single ulp error can translate into a long time difference
        // depending on the orbits so we check that the distance is consistent
        {
            double t = orbit_time_at_distance(o, o->periapsis);
            assertIsClose(orbit_distance_at_time(o, t), o->periapsis);
        }
        if (o->eccentricity < 1.) {  // closed orbit
            double t = orbit_time_at_distance(o, o->apoapsis);
            assertIsClose(orbit_distance_at_time(o, t), o->apoapsis);
        }
    }

    // escape
    assertIsClose(orbit_time_at_distance(o, o->primary->sphere_of_influence), orbit_time_at_escape(o));
    double time_at_escape = orbit_time_at_escape(o);
    if (!isnan(time_at_escape)) {
        assertIsClose(glm::distance(orbit_position_at_escape(o), orbit_position_at_time(o, time_at_escape)), 0.f);
        assertIsClose(glm::distance(orbit_velocity_at_escape(o), orbit_velocity_at_time(o, time_at_escape)), 0.f);
    }
}

static void test_orbits(void) {
    {
        Orbit orbit;
        orbit_from_periapsis(&orbit, &primary, 1e6, .5);
        orbit_orientate(&orbit, 0., 0., 0., 0., 0.);
        //glm::dvec3 pos;
        //orbit_position_at_time(pos, &orbit, 0.);
        //assert(glm::dvec3_norm(pos) != 0.);
    }

    double periapses[] = {1e9, 1e13};
    double d = 1e-5;
    double eccentricities[] = {0., d, .5, 1.-d, 1., 1.+d, 10., 100.};
    // try every combination of the values above as orbital parameters
    for (size_t i = 0; i < countof(periapses); i += 1) {
        for (size_t j = 0; j < countof(eccentricities); j += 1) {
            Orbit orbit;
            orbit_from_periapsis(&orbit, &primary, periapses[i], eccentricities[j]);
            for (size_t k = 0; k < countof(angle_testset); k += 1) {
                for (size_t l = 0; l < countof(angle_testset); l += 1) {
                    for (size_t m = 0; m < countof(angle_testset); m += 1) {
                        orbit_orientate(&orbit, angle_testset[k], angle_testset[l], angle_testset[m], 0., 0.);
                        test_orbit(&orbit);
                    }
                }
            }
        }
    }
}

static void test_orbit_invalid(void) {
    Orbit orbit;

    // closed orbit should have positive eccentricity
    assertFails(orbit_from_periapsis(&orbit, &primary, 1e9, -.5));

    // closed orbit should have positive semi-major axis
    assertFails(orbit_from_semi_major(&orbit, &primary, -1e9, 0.));

    // hyperbolic trajectory should have negative semi-major axis
    assertFails(orbit_from_semi_major(&orbit, &primary, 1e9, 2.));

    // parabolic trajectory cannot be defined from semi-major axis
    assertFails(orbit_from_semi_major(&orbit, &primary, 1e9, 1.));
    assertFails(orbit_from_semi_major(&orbit, &primary, -1e9, 1.));

    // parabolic trajectory cannot be defined from period
    assertFails(orbit_from_period(&orbit, &primary, 1e8, 1.));
    assertFails(orbit_from_period2(&orbit, &primary, INFINITY, 1e9));
    assertFails(orbit_from_period2(&orbit, &primary, -INFINITY, 1e9));
}

static void test_load_solar_system(void) {
    Dict solar_system;
    if (load_bodies(&solar_system, "data/solar_system.json") < 0) {
        fprintf(stderr, "Failed to load '%s'\n", "data/solar_system.json");
        exit(EXIT_FAILURE);
    }

    assert(solar_system.count("Sun")     == 1);
    assert(solar_system.count("Mercury") == 1);
    assert(solar_system.count("Venus")   == 1);
    assert(solar_system.count("Earth")   == 1);
    assert(solar_system.count("Moon")    == 1);
    assert(solar_system.count("Mars")    == 1);
    assert(solar_system.count("Jupiter") == 1);
    assert(solar_system.count("Saturn")  == 1);
    assert(solar_system.count("Uranus")  == 1);
    assert(solar_system.count("Neptune") == 1);
    assert(solar_system.count("XXX")     == 0);

    CelestialBody* sun = solar_system["Sun"];
    CelestialBody* earth = solar_system["Earth"];
    CelestialBody* moon = solar_system["Moon"];
    if (sun != NULL) {
        assert(std::string(sun->name) == std::string("Sun"));
        assert(sun->n_satellites >= 8);
    }
    if (sun != NULL && earth != NULL) {
        assertIsLower(earth->mass, sun->mass);
        assert(earth->orbit->primary == sun);
    }
    if (earth != NULL && moon != NULL) {
        assertIsLower(moon->mass, earth->mass);
        assert(moon->orbit->primary == earth);
    }

    unload_bodies(&solar_system);
}

static void test_load_kerbol_system(void) {
    Dict kerbol_system;
    if (load_bodies(&kerbol_system, "data/kerbol_system.json") < 0) {
        fprintf(stderr, "Failed to load '%s'\n", "data/kerbol_system.json");
        exit(EXIT_FAILURE);
    }

    assert(kerbol_system.count("Kerbol") == 1);
    assert(kerbol_system.count("Moho")   == 1);
    assert(kerbol_system.count("Eve")    == 1);
    assert(kerbol_system.count("Kerbin") == 1);
    assert(kerbol_system.count("Mun")    == 1);
    assert(kerbol_system.count("Minmus") == 1);
    assert(kerbol_system.count("Duna")   == 1);
    assert(kerbol_system.count("Dres")   == 1);
    assert(kerbol_system.count("Jool")   == 1);
    assert(kerbol_system.count("Eeloo")  == 1);
    assert(kerbol_system.count("XXX")    == 0);

    CelestialBody* kerbol = kerbol_system["Kerbol"];
    CelestialBody* kerbin = kerbol_system["Kerbin"];
    CelestialBody* mun = kerbol_system["Mun"];
    if (kerbol != NULL) {
        assert(std::string(kerbol->name) == std::string("Kerbol"));
        assert(kerbol->n_satellites == 7);
    }
    if (kerbol != NULL && kerbin != NULL) {
        assertIsLower(kerbin->mass, kerbol->mass);
        assert(kerbin->orbit->primary == kerbol);
    }
    if (kerbin != NULL && mun != NULL) {
        assertIsLower(mun->mass, kerbin->mass);
        assert(mun->orbit->primary == kerbin);
    }

    unload_bodies(&kerbol_system);
}

static void test_load(void) {
    test_load_solar_system();
    test_load_kerbol_system();
}

static void test_recipes(void) {
    // dummy object
    CelestialBody kerbin = make_dummy_object(600e3, 3.5316e+12, 0);

    // darkness time
    Orbit orbit;
    // from <https://wiki.kerbalspaceprogram.com/wiki/Orbit_darkness_time>
    orbit_from_periapsis(&orbit, &kerbin, kerbin.radius+120e3, 0);
    assertIsClose(darkness_time(&orbit), 640.5131630404287);

    // synodic period
    // from <https://en.wikipedia.org/wiki/Orbital_period#Examples_of_sidereal_and_synodic_periods>
    Orbit mercury = make_dummy_orbit_with_period(0.240846);
    Orbit venus = make_dummy_orbit_with_period(0.615);
    Orbit earth = make_dummy_orbit_with_period(1.);
    Orbit moon = make_dummy_orbit_with_period(0.0748);
    Orbit mars = make_dummy_orbit_with_period(1.881);
    assertIsLower(fabs(synodic_period(&earth, &mercury) - 0.317),  1e-3);
    assertIsLower(fabs(synodic_period(&earth, &venus)   - 1.598),  1e-3);
    assertIsLower(fabs(synodic_period(&earth, &moon)    - 0.0809), 1e-4);
    assertIsLower(fabs(synodic_period(&earth, &mars)    - 2.135),  1e-4);

    // satellite constellations
    // minimum size
    assert(constellation_minimum_size(&kerbin, 1e12) == 3);
    assert(constellation_minimum_size(&kerbin, 1200e3) == 4);
    assert(constellation_minimum_size(&kerbin, 500e3) == 8);
    assert(constellation_minimum_size(&kerbin, -500e3) == 0);
    // minimum radius
    assertIsLower(kerbin.radius, constellation_minimum_radius(&kerbin, 3));
    assertIsLower(kerbin.radius, constellation_minimum_radius(&kerbin, 10));
    assertIsClose(kerbin.radius, constellation_minimum_radius(&kerbin, (unsigned) -1));
    // maximum radius
    assertIsLower(constellation_minimum_radius(&kerbin, 3), constellation_maximum_radius(1e12, 3));
    assertIsLower(constellation_minimum_radius(&kerbin, 4), constellation_maximum_radius(1200e3, 4));
    // too few satellites
    assertIsLower(constellation_maximum_radius(1200e3, 3), constellation_minimum_radius(&kerbin, 3));

    // circular orbital speed
    assertIsClose(circular_orbit_speed(&primary, INFINITY), 0.);
    assertEquals(circular_orbit_speed(&primary, 0.), INFINITY);
    assertIsClose(body_escape_velocity(&primary, 100e3), sqrt(2.) * circular_orbit_speed(&primary, 100e3));

    // maneuvers
    double r1 = 700e3;
    double r2 = 1700e3;
    // check that Hohmann transfer is a particular bi-elliptical transfer
    assertIsClose(maneuver_hohmann_cost(&primary, r1, r2), maneuver_bielliptic_cost(&primary, r1, r2, r2));
    assertIsClose(maneuver_hohmann_time(&primary, r1, r2), maneuver_bielliptic_time(&primary, r1, r2, r2));
    // check worst ratio of Hohmann transfer
    assertIsLower(maneuver_hohmann_cost(&primary, r1, r1*(HOHMANN_WORST_RATIO-1e-6)), maneuver_hohmann_cost(&primary, r1, r1*HOHMANN_WORST_RATIO));
    assertIsLower(maneuver_hohmann_cost(&primary, r1, r1*(HOHMANN_WORST_RATIO+1e-6)), maneuver_hohmann_cost(&primary, r1, r1*HOHMANN_WORST_RATIO));
    // compare efficiency of bi-elliptical transfer with Hohmann transfer
    r2 = r1*50.;
    assertIsLower(maneuver_hohmann_cost(&primary, r1, r2), maneuver_bielliptic_cost(&primary, r1, r2, r1*40.));
    assertIsLower(maneuver_hohmann_time(&primary, r1, r2), maneuver_bielliptic_time(&primary, r1, r2, r1*40.));
    assertIsLower(maneuver_bielliptic_cost(&primary, r1, r2, r1*60.), maneuver_hohmann_cost(&primary, r1, r2));
    assertIsLower(maneuver_hohmann_time(&primary, r1, r2), maneuver_bielliptic_time(&primary, r1, r2, r1*60.));
    r2 = r1*9999.;
    assertIsLower(maneuver_bielliptic_cost(&primary, r1, r2, INFINITY), maneuver_hohmann_cost(&primary, r1, r2));
    assertEquals(maneuver_bielliptic_time(&primary, r1, r2, INFINITY), INFINITY);
    // check that Hohmann transfer is a particular one-tangent burn
    double e = maneuver_one_tangent_burn_minimum_eccentricity(r1, r2);
    assertIsClose(maneuver_one_tangent_burn_time(&primary, r1, r2, e), maneuver_hohmann_time(&primary, r1, r2));
    assertIsClose(maneuver_one_tangent_burn_cost(&primary, r1, r2, e), maneuver_hohmann_cost(&primary, r1, r2));
    // plane change
    assertIsClose(maneuver_plane_change_cost(100., 0), 0);
    assertIsClose(maneuver_plane_change_cost(100., M_PI/2.), 100.*sqrt(2.));
    assertIsClose(maneuver_plane_change_cost(100., M_PI), 200);
    // inclination change
    {
        Orbit o;
        orbit_from_periapsis(&o, &primary, 700e3, 0.);
        orbit_orientate(&o, 0., 0., 0., 0., 0.);
        double speed = orbit_speed_at_distance(&o, o.semi_major_axis);
        assertIsClose(maneuver_plane_change_cost(speed, 2.), maneuver_inclination_change_cost(&o, 0., 2.));
    }
    // TODO: more tests for inclination change
}

void test_lambert(void) {
    // <http://www.braeunig.us/space/problem.htm#5.4>
    {
        const double au = 1.4959787e+11;  // astronomical unit
        glm::dvec3 r1{0.473265*au, -0.899215*au, 0.};
        glm::dvec3 r2{0.066842*au, 1.561256*au, 0.030948*au};
        double mu = 1.327124e20;
        double t = 207. * 86400.;
        glm::dvec3 v1, v2;
        lambert(v1, v2, mu, r1, r2, t, 0, 0);
        assertIsClose(v1[0], 28996.23493547104);
        assertIsClose(v1[1], 15232.684101572762);
        assertIsClose(v1[2], 1289.1732573653683);
        assertIsClose(v2[0], -21147.045109982573);
        assertIsClose(v2[1], 3994.4133718239927);
        assertIsClose(v2[2], -663.3280036013251);
    }

    // Fundamentals of Astrodynamics, p236
    {
        glm::dvec3 r1{.5, .6, .7};
        glm::dvec3 r2{0., 1., 0.};
        double mu = 1.;
        double t = (445. - 432.) / 13.44686457;  // see appendix A
        glm::dvec3 v1, v2;
        // short way
        lambert(v1, v2, mu, r1, r2, t, 0, 0);
        assertIsClose(v1[0], -0.36163780780789323);
        assertIsClose(v1[1], 0.7697267599186077);
        assertIsClose(v1[2], -0.5062929309310507);
        assertIsClose(v2[0], -0.6018460646440396);
        assertIsClose(v2[1], -0.02238823863132538);
        assertIsClose(v2[2], -0.8425844905016555);
        // long way (note the inverted signs)
        lambert(v2, v1, mu, r2, r1, t, 0, 0);
        assertIsClose(-v1[0], -0.6305417321526077);
        assertIsClose(-v1[1], -1.1139628156077221);
        assertIsClose(-v1[2], -0.8827584250136509);
        assertIsClose(-v2[0], +0.17865636851229638);
        assertIsClose(-v2[1], +1.5544631609898276);
        assertIsClose(-v2[2], +0.25011891591721497);  // typo in book
    }
}

void test_rk4(void) {
    // dummy object
    CelestialBody earth = make_dummy_object(6371e3, 3.98601e+14, 0);

    double time = 0;
    static const double SIMULATION_STEP = 1. / 128.;

    Rocket rocket;
    rocket.name = "Rocket";
    rocket.state = {
        glm::dvec3{6371e3 + 300e3, 0, 0},
        glm::dvec3{0, 7660, 0},
    };

    Orbit orbit;
    orbit_from_state(&orbit, &earth, rocket.state.position, rocket.state.velocity, time);
    rocket.orbit = &orbit;

    for (size_t i = 0; i < 1<<20; i += 1) {
        rocket_update(&rocket, time, SIMULATION_STEP, 0.);
        time += SIMULATION_STEP;
    }

    auto numint_pos = rocket.state.position;
    auto kepler_pos = orbit_position_at_time(rocket.orbit, time);
    auto relative_error = glm::distance(numint_pos, kepler_pos) / glm::length(kepler_pos);
    assertIsClose(relative_error, 0.);
}

int main(void) {
    test_coordinates();    printf("."); fflush(stdout);
    test_body();           printf("."); fflush(stdout);
    test_orbits();         printf("."); fflush(stdout);
    test_orbit_invalid();  printf("."); fflush(stdout);
    test_load();           printf("."); fflush(stdout);
    test_recipes();        printf("."); fflush(stdout);
    test_lambert();        printf("."); fflush(stdout);
    test_rk4();            printf("."); fflush(stdout);
    printf("\n");
}
