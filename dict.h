#ifndef DICT_H
#define DICT_H

typedef struct dict Dict;

#include <stddef.h>

struct dict {
    size_t n_slots;
    size_t a_slots;
    char** keys;
    void** values;
};

void dict_init(Dict* dict);
void dict_free(Dict* dict);

void  dict_set(Dict* dict, const char* key, void* value);
void  dict_del(Dict* dict, const char* key);
void* dict_get(Dict* dict, const char* key);

#endif
