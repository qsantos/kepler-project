#include "load.hpp"

#include "body.hpp"

#include <cstring>

#include <cjson/cJSON.h>

extern "C" {
#include "util.h"
#include "logging.h"
}

static double get_param_required(cJSON* json, const char* object_name, const char* param_name);
static double get_param_optional(cJSON* json, const char* object_name, const char* param_name);

static CelestialBody* parse_body(Dict* bodies, cJSON* jbodies, const char* name);
static CelestialCoordinates* parse_coordinates(cJSON* jcoordinates, const char* body_name);
static Orbit* parse_orbit(Dict* bodies, cJSON* jbodies, cJSON* jorbit, const char* body_name);

static double get_param_required(cJSON* json, const char* object_name, const char* param_name) {
    cJSON* jparam = cJSON_GetObjectItemCaseSensitive(json, param_name);
    if (jparam == NULL) {
        CRITICAL("'%s' is missing required parameter '%s'", object_name, param_name);
        exit(EXIT_FAILURE);
    }
    if (!cJSON_IsNumber(jparam)) {
        CRITICAL("The required parameter '%s' of '%s' is not a number", param_name, object_name);
        exit(EXIT_FAILURE);
    }
    return jparam->valuedouble;
}

static double get_param_optional(cJSON* json, const char* object_name, const char* param_name) {
    cJSON* jparam = cJSON_GetObjectItemCaseSensitive(json, param_name);
    if (jparam == NULL) {
        return 0.;
    }
    if (!cJSON_IsNumber(jparam)) {
        ERROR("The optional parameter '%s' of '%s' is not a number", param_name, object_name);
        return 0.;
    }
    return jparam->valuedouble;
}

static Orbit* parse_orbit(Dict* bodies, cJSON* jbodies, cJSON* jorbit, const char* body_name) {
    if (jorbit == NULL) {
        return NULL;
    }

    // primary
    cJSON* jprimary = cJSON_GetObjectItemCaseSensitive(jorbit, "primary");
    if (jprimary == NULL) {
        CRITICAL("'%s' has an orbit but no primary", body_name);
        exit(EXIT_FAILURE);
    }
    if (!cJSON_IsString(jprimary)) {
        CRITICAL("The name of the primary of '%s' is not a string", body_name);
        exit(EXIT_FAILURE);
    }
    CelestialBody* primary = parse_body(bodies, jbodies, jprimary->valuestring);

    double semi_major_axis             = get_param_required(jorbit, body_name, "semi_major_axis");
    double eccentricity                = get_param_optional(jorbit, body_name, "eccentricity");
    double longitude_of_ascending_node = get_param_optional(jorbit, body_name, "longitude_of_ascending_node");
    double inclination                 = get_param_optional(jorbit, body_name, "inclination");
    double argument_of_periapsis       = get_param_optional(jorbit, body_name, "argument_of_periapsis");
    double epoch                       = get_param_optional(jorbit, body_name, "epoch");
    double mean_anomaly_at_epoch       = get_param_optional(jorbit, body_name, "mean_anomaly_at_epoch");

    // create Orbit object
    Orbit* ret = new Orbit;
    orbit_from_semi_major(ret, primary, semi_major_axis, eccentricity);
    orbit_orientate(ret, longitude_of_ascending_node, inclination, argument_of_periapsis, epoch, mean_anomaly_at_epoch);
    return ret;
}

static CelestialCoordinates* parse_coordinates(cJSON* jcoordinates, const char* body_name) {
    if (jcoordinates == NULL) {
        return NULL;
    }

    double right_ascension = get_param_required(jcoordinates, body_name, "right_ascension");
    double declination     = get_param_required(jcoordinates, body_name, "declination");
    double distance        = get_param_optional(jcoordinates, body_name, "distance");
    CelestialCoordinates* coordinates = new CelestialCoordinates;
    *coordinates = CelestialCoordinates::from_equatorial(right_ascension, declination, distance);
    return coordinates;
}

static CelestialBody* parse_body(Dict* bodies, cJSON* jbodies, const char* name) {
    auto search = bodies->find(name);
    if (search != bodies->end()) {
        return search->second;
    }

    cJSON* jbody = cJSON_GetObjectItemCaseSensitive(jbodies, name);
    if (jbody == NULL) {
        CRITICAL("Body '%s' not found", name);
        exit(EXIT_FAILURE);
    }

    CelestialBody* ret = new CelestialBody;
    (*bodies)[name] = ret;
    body_init(ret);
    body_set_name(ret, strdup(name));

    double radius = get_param_optional(jbody, name, "radius");
    if (radius != 0.) {
        body_set_radius(ret, radius);
    } else {
        WARNING("'%s' has no radius!", name);
    }

    double gravitational_parameter = get_param_optional(jbody, name, "gravitational_parameter");
    double mass = get_param_optional(jbody, name, "mass");
    if (gravitational_parameter != 0.) {
        body_set_gravparam(ret, gravitational_parameter);
    } else if (mass != 0.) {
        body_set_mass(ret, mass);
    } else {
        WARNING("'%s' has neither mass or gravitational_parameter", name);
    }

    double rotational_period = get_param_optional(jbody, name, "rotational_period");
    if (rotational_period != 0.) {
        body_set_rotation(ret, rotational_period);
    }

    cJSON* jpositive_pole = cJSON_GetObjectItemCaseSensitive(jbody, "positive_pole");
    CelestialCoordinates* positive_pole = parse_coordinates(jpositive_pole, name);
    if (positive_pole != NULL) {
        body_set_axis(ret, positive_pole);
    }

    cJSON* jorbit = cJSON_GetObjectItemCaseSensitive(jbody, "orbit");
    body_set_orbit(ret, parse_orbit(bodies, jbodies, jorbit, name));

    return ret;
}

int parse_bodies(Dict* bodies, const char* json) {
    cJSON* jbodies = cJSON_Parse(json);
    if (jbodies == NULL) {
        CRITICAL("Failed to parse JSON (%s)", cJSON_GetErrorPtr());
        return -1;
    }

    if (!cJSON_IsObject(jbodies)) {
        CRITICAL("Expected JSON object at root");
        return -1;
    }

    for (cJSON* body = jbodies->child; body != NULL; body = body->next) {
        parse_body(bodies, jbodies, body->string);
    }

    cJSON_Delete(jbodies);
    return 0;
}

int load_bodies(Dict* bodies, const char* filename) {
    char* json = load_file(filename);
    if (json == NULL) {
        CRITICAL("Failed to open '%s'", filename);
        return -1;
    }
    int ret = parse_bodies(bodies, json);
    free(json);
    return ret;
}

void unload_bodies(Dict* bodies) {
    for (auto i : *bodies) {
        body_clear(i.second);
    }
}
