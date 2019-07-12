#include "dict.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

static char* _strdup(const char* str) {
    return strcpy(MALLOC(strlen(str) + 1), str);
}

static ssize_t _dict_find(Dict* dict, const char* key) {
    for (size_t i = 0; i < dict->n_slots; i += 1) {
        if (strcmp(key, dict->keys[i]) == 0) {
            return (ssize_t) i;
        }
    }
    return -1;
}

void dict_init(Dict* dict) {
    dict->n_slots = 0;
    dict->a_slots = 0;
    dict->keys = NULL;
    dict->values = NULL;
}

void dict_free(Dict* dict) {
    for (size_t i = 0; i < dict->n_slots; i += 1) {
        free(dict->keys[i]);
    }
    free(dict->values);
    free(dict->keys);
}

void dict_set(Dict* dict, const char* key, void* value) {
    ssize_t i = _dict_find(dict, key);
    if (i < 0) {
        if (dict->a_slots == dict->n_slots) {
            dict->a_slots = dict->a_slots == 0 ? 1 : 2*dict->a_slots;
            dict->keys = REALLOC(dict->keys, sizeof(char*) * dict->a_slots); 
            dict->values = REALLOC(dict->values, sizeof(void*) * dict->a_slots); 
        }
        i = (ssize_t) dict->n_slots;
        dict->n_slots += 1;
    }
    dict->keys[i] = _strdup(key);
    dict->values[i] = value;
}

void dict_del(Dict* dict, const char* key) {
    ssize_t i = _dict_find(dict, key);
    if (i < 0) {
        return;
    }
    dict->n_slots -= 1;
    if (dict->n_slots != 0) {
        dict->keys[i] = dict->keys[dict->n_slots];
        dict->values[i] = dict->values[dict->n_slots];
    }
}

void* dict_get(Dict* dict, const char* key) {
    ssize_t i = _dict_find(dict, key);
    if (i < 0) {
        return NULL;
    }
    return dict->values[i];
}
