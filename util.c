#include "util.h"

#include <stdio.h>
#include <string.h>

void* _malloc(size_t s, const char* filename, int line, const char* funcname) {
    if (s == 0) {
        return NULL;
    }
    void* ret = malloc(s);
    if (ret == NULL) {
        fprintf(stderr, "Failed to allocate %zu bytes at %s:%i in %s\n", s, filename, line, funcname);
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
        fprintf(stderr, "Failed to allocate %zu bytes at %s:%i in %s\n", s, filename, line, funcname);
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
        s += snprintf(ret, n, "%.0f,%.0f %s%s", thousands, ones, prefix, unit);
    }
    else if (v > 1e2) { s += snprintf(ret, n, "%.1f %s%s", v, prefix, unit); }
    else if (v > 1e1) { s += snprintf(ret, n, "%.2f %s%s", v, prefix, unit); }
    else if (v > 1e0) { s += snprintf(ret, n, "%.3f %s%s", v, prefix, unit); }
    else              { s += snprintf(ret, n, "%.4f %s%s", v, prefix, unit); }

    // additional information
    if (value > ly) {
        s += snprintf(ret + s, n - (size_t) s, " (%.0f ly)", value / ly);
    }

    return ret;
}
