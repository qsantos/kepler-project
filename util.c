#include "util.h"

#include <stdio.h>
#include <string.h>

#include "logging.h"

void* _malloc(size_t s, const char* filename, int line, const char* funcname) {
    if (s == 0) {
        return NULL;
    }
    void* ret = malloc(s);
    if (ret == NULL) {
        CRITICAL("Failed to allocate %zu bytes at %s:%i in %s", s, filename, line, funcname);
        exit(EXIT_FAILURE);
    }
    return ret;
}

void* _realloc(void* p, size_t s, const char* filename, int line, const char* funcname) {
    if (s == 0) {
        return NULL;
    }
    void* ret = realloc(p, s);
    if (ret == NULL) {
        CRITICAL("Failed to allocate %zu bytes at %s:%i in %s", s, filename, line, funcname);
        exit(EXIT_FAILURE);
    }
    return ret;
}

char* load_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        perror("while opening file");
        return NULL;
    }

    // get file size
    if (fseek(f, 0, SEEK_END) < 0) {
        perror("while seeking end of file");
        fclose(f);
        return NULL;
    }
    long length = ftell(f);
    if (length < 0) {
        perror("while reading file");
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) < 0) {
        perror("while seeking start of file");
        fclose(f);
        return NULL;
    }

    // read file
    char* ret = (char*) MALLOC((size_t) length + 1);
    if (fread(ret, 1, (size_t) length, f) < (size_t) length) {
        perror("incomplete read");
    }

    // finish
    ret[length] = '\0';
    if (fclose(f) < 0) {
        perror("failed to close file");
    }
    return ret;
}

char* human_quantity(double value, const char* unit) {
    size_t n = 100 + strlen(unit);
    char* ret = (char*) MALLOC(n);

    static const double c = 299792458.;
    static const double ly = c * 365.25 * 86400.;

    // choose SI prefix
    double v = value;
    const char* prefix;
         if (v > 10e24) { prefix = "Y"; v /= 1e24; }
    else if (v > 10e21) { prefix = "Z"; v /= 1e21; }
    else if (v > 10e18) { prefix = "E"; v /= 1e18; }
    else if (v > 10e15) { prefix = "P"; v /= 1e15; }
    else if (v > 10e12) { prefix = "T"; v /= 1e12; }
    else if (v > 10e09) { prefix = "G"; v /= 1e09; }
    else if (v > 10e06) { prefix = "M"; v /= 1e06; }
    else if (v > 10e03) { prefix = "k"; v /= 1e03; }
    else { prefix = ""; }

    // precision with SI prefix
    int s = 0;
    if (v >= 1e3) {
        // handle single comma
        double thousands = floor(v / 1e3);
        double ones = v - 1e3 * thousands;
        s += snprintf(ret, n, "%.0f,%03.0f %s%s", thousands, ones, prefix, unit);
    }
    else if (v > 1e2) { s += snprintf(ret, n, "%.1f %s%s", v, prefix, unit); }
    else if (v > 1e1) { s += snprintf(ret, n, "%.2f %s%s", v, prefix, unit); }
    else if (v > 1e0) { s += snprintf(ret, n, "%.3f %s%s", v, prefix, unit); }
    else              { s += snprintf(ret, n, "%.4f %s%s", v, prefix, unit); }

    // additional information
    if (value > ly) {
        snprintf(ret + s, n - (size_t) s, " (%.0f ly)", value / ly);
    }

    return ret;
}

size_t count(const char* s, const char* pattern) {
    size_t matches = 0;
    if (s == NULL || pattern == NULL) {
        return 0;
    }

    size_t n_pattern = strlen(pattern);
    while (1) {
        s = strstr(s, pattern);
        if (s == NULL) {
            break;
        }

        matches += 1;
        s += n_pattern;;
    }

    return matches;
}

char* replace(const char* s, const char* pattern, const char* replacement) {
    count(NULL, pattern);

    size_t n_s = strlen(s);
    size_t n_pattern = strlen(pattern);
    size_t n_replacement = strlen(replacement);

    size_t n = n_s + count(s, pattern) * (n_replacement - n_pattern) + 1;
    char* ret = MALLOC(n);

    char* cur = ret;
    while (1) {
        // find pattern
        const char* match = strstr(s, pattern);
        if (match == NULL) {
            break;
        }

        // copy s until pattern
        size_t n_prefix = (size_t) (match - s);
        memcpy(cur, s, n_prefix);
        s = match;
        cur += n_prefix;

        // copy replacement
        memcpy(cur, replacement, n_replacement);
        s += n_pattern;
        cur += n_replacement;
    }

    // copy rest of s
    memcpy(cur, s, strlen(s));
    ret[n - 1] = '\0';

    return ret;
}

#ifdef MSYS2
char* strdup(const char* s) {
    size_t n = strlen(s);

    char* r = malloc(n + 1);
    if (r == NULL) {
        return NULL;
    }

    memcpy(r, s, n + 1);
    return r;
}
#endif
