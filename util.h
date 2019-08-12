#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PIf32
#define M_PIf32 3.14159265358979323846f
#endif

inline double fmod2(double x, double y) {
    double r = fmod(x, y);
    if (r < 0.) {
        r += y;
    }
    return r;
}

inline double degrees(double rad) {
    return rad * 180. / M_PI;
}

inline double radians(double deg) {
    return deg * M_PI / 180.;
}

inline double tan_(double x) {
    // the actual tan() can be incredibly slow around π/2
    return sin(x) / cos(x);
}

inline double random_uniform(double a, double b) {
    return a + (double)rand() / RAND_MAX * (b -  a);
}

inline int isclose(double a, double b) {
    double rel_tol=1e-9;
    double abs_tol=1e-9;
    return fabs(a-b) <= fmax(rel_tol * fmax(fabs(a), fabs(b)), abs_tol);
}

void* _malloc(size_t s, const char* filename, int line, const char* funcname);
void* _realloc(void* p, size_t s, const char* filename, int line, const char* funcname);

#define MALLOC(s) _malloc(s, __FILE__, __LINE__, __func__)
#define REALLOC(p, s) _realloc(p, s, __FILE__, __LINE__, __func__)

inline double real_clock(void) {
    /* Clock that measures wall-clock time */
    struct timeval now;
    gettimeofday(&now, NULL);
    return (double) now.tv_sec + (double) now.tv_usec / 1e6;
}

char* load_file(const char* filename);
char* human_quantity(double v, const char* unit);

size_t count(const char* s, const char* pattern);
char* replace(const char* s, const char* pattern, const char* replacement);

#ifdef MSYS2
char* strdup(const char* s);
#endif

#endif
