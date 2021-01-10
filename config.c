#include "config.h"

#include "util.h"
#include "logging.h"

#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

char* strdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* ret = malloc(n);
    if (ret == NULL) { abort(); }
    memcpy(ret, s, n);
    return ret;
}

static char* get_string(cJSON* json, const char* system_id, const char* param_name) {
    cJSON* jparam = cJSON_GetObjectItemCaseSensitive(json, param_name);
    if (jparam == NULL) {
        CRITICAL("System '%s' is missing required parameter '%s'", system_id, param_name);
        exit(EXIT_FAILURE);
    }
    return strdup(jparam->valuestring);
}

static double get_number(cJSON* json, const char* system_id, const char* param_name) {
    cJSON* jparam = cJSON_GetObjectItemCaseSensitive(json, param_name);
    if (jparam == NULL) {
        CRITICAL("System '%s' is missing required parameter '%s'", system_id, param_name);
        exit(EXIT_FAILURE);
    }
    if (!cJSON_IsNumber(jparam)) {
        CRITICAL("The required parameter '%s' of '%s' is not a number", param_name, system_id);
        exit(EXIT_FAILURE);
    }
    return jparam->valuedouble;
}

struct SystemConfig load_system_config(cJSON* config, const char* system_id) {
    cJSON* systems = cJSON_GetObjectItemCaseSensitive(config, "systems");
    if (systems == NULL) {
        CRITICAL("Config is missing the systems section");
        exit(EXIT_FAILURE);
    }

    cJSON* system = cJSON_GetObjectItem(systems, system_id); // case insensitive
    if (system == NULL) {
        CRITICAL("No system '%s' found in config", system_id);
        exit(EXIT_FAILURE);
    }

    return (struct SystemConfig) {
        .default_focus      = get_string(system, system_id, "default_focus"),
        .display_name       = get_string(system, system_id, "display_name"),
        .root               = get_string(system, system_id, "root"),
        .spaceship_altitude = get_number(system, system_id, "spaceship_altitude"),
        .star_temperature   = get_number(system, system_id, "star_temperature"),
        .system_data        = get_string(system, system_id, "system_data"),
        .textures_directory = get_string(system, system_id, "textures_directory"),
    };
}

struct Config load_config(const char* filename, const char* system_id) {
    char* json = load_file(filename);
    if (json == NULL) {
        CRITICAL("Failed to open '%s'", filename);
        exit(EXIT_FAILURE);
    }

    cJSON* config = cJSON_Parse(json);
    if (config == NULL) {
        CRITICAL("Failed to parse JSON (%s)", cJSON_GetErrorPtr());
        exit(EXIT_FAILURE);
    }

    struct Config ret;
    ret.system = load_system_config(config, system_id);
    free(json);
    return ret;
}

void delete_config(struct Config* config) {
    free(config->system.default_focus);
    free(config->system.display_name);
    free(config->system.root);
    free(config->system.system_data);
    free(config->system.textures_directory);
}
