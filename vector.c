#include "vector.h"

#include <string.h>

#include "util.h"

double vec3_unorm(const Vec3 u) {
    double u0 = fabs(u[0]);
    double u1 = fabs(u[1]);
    double u2 = fabs(u[2]);
    return fmax(u0, fmax(u1, u2));
}

double vec3_norm(const Vec3 u) {
    return sqrt(vec3_dot(u, u));
}

double vec3_udist(const Vec3 u, const Vec3 v) {
    Vec3 s;
    vec3_sub(s, u, v);
    return vec3_unorm(s);
}

double vec3_dist(const Vec3 u, const Vec3 v) {
    Vec3 s;
    vec3_sub(s, u, v);
    return vec3_norm(s);
}

double vec3_dot(const Vec3 u, const Vec3 v) {
    return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

double vec3_angle(const Vec3 u, const Vec3 v) {
    double r = vec3_dot(u, v) / (vec3_norm(u) * vec3_norm(v));
    r = fmax(-1., fmin(1., r));
    return acos(r);
}

double vec3_angle2(const Vec3 u, const Vec3 v, const Vec3 normal) {
    Vec3 default_normal = {0., 0., 1.};
    if (normal == NULL) {
        normal = default_normal;
    }

    double geometric_angle = vec3_angle(u, v);

    Vec3 u_cross_v;
    vec3_cross(u_cross_v, u, v);
    if (vec3_dot(normal, u_cross_v) < 0.) {
        return -geometric_angle;
    } else {
        return geometric_angle;
    }
}

void vec3_add(Vec3 res, const Vec3 u, const Vec3 v) {
    res[0] = u[0] + v[0];
    res[1] = u[1] + v[1];
    res[2] = u[2] + v[2];
}

void vec3_sub(Vec3 res, const Vec3 u, const Vec3 v) {
    res[0] = u[0] - v[0];
    res[1] = u[1] - v[1];
    res[2] = u[2] - v[2];
}

void vec3_scale(Vec3 res, const Vec3 u, double s) {
    res[0] = u[0] * s;
    res[1] = u[1] * s;
    res[2] = u[2] * s;
}

void vec3_cross(Vec3 res, const Vec3 u, const Vec3 v) {
    double u0 = u[0], u1 = u[1], u2 = u[2];
    double v0 = v[0], v1 = v[1], v2 = v[2];
    res[0] = u1*v2 - u2*v1;
    res[1] = u2*v0 - u0*v2;
    res[2] = u0*v1 - u1*v0;
}

double mat3_unorm(Mat3 a) {
    double a0 = vec3_unorm(a[0]);
    double a1 = vec3_unorm(a[1]);
    double a2 = vec3_unorm(a[2]);
    return fmax(a0, fmax(a1, a2));
}

double mat3_udist(Mat3 a, Mat3 b) {
    Mat3 c;
    mat3_sub(c, a, b);
    return mat3_unorm(c);
}

void mat3_add(Mat3 res, Mat3 a, Mat3 b) {
    vec3_add(res[0], a[0], b[0]);
    vec3_add(res[1], a[1], b[1]);
    vec3_add(res[2], a[2], b[2]);
}

void mat3_sub(Mat3 res, Mat3 a, Mat3 b) {
    vec3_sub(res[0], a[0], b[0]);
    vec3_sub(res[1], a[1], b[1]);
    vec3_sub(res[2], a[2], b[2]);
}

void mat3_mulv(Vec3 res, Mat3 a, Vec3 b) {
    Vec3 c = {b[0], b[1], b[2]};
    res[0] = vec3_dot(a[0], c);
    res[1] = vec3_dot(a[1], c);
    res[2] = vec3_dot(a[2], c);
}

void mat3_mulm(Mat3 res, Mat3 a, Mat3 b) {
    Mat3 tmp;
    tmp[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0];
    tmp[0][1] = a[0][0]*b[0][1] + a[0][1]*b[1][1] + a[0][2]*b[2][1];
    tmp[0][2] = a[0][0]*b[0][2] + a[0][1]*b[1][2] + a[0][2]*b[2][2];
    tmp[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0];
    tmp[1][1] = a[1][0]*b[0][1] + a[1][1]*b[1][1] + a[1][2]*b[2][1];
    tmp[1][2] = a[1][0]*b[0][2] + a[1][1]*b[1][2] + a[1][2]*b[2][2];
    tmp[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0];
    tmp[2][1] = a[2][0]*b[0][1] + a[2][1]*b[1][1] + a[2][2]*b[2][1];
    tmp[2][2] = a[2][0]*b[0][2] + a[2][1]*b[1][2] + a[2][2]*b[2][2];
    memcpy(res, tmp, sizeof(Mat3));
}

void mat3_from_angle_axis(Mat3 res, double angle, double x, double y, double z) {
    double s = sin(angle);
    double c = cos(angle);
    double d = sqrt(x*x + y*y + z*z);
    x /= d;
    y /= d;
    z /= d;
    res[0][0] = x*x*(1.-c) + c;
    res[0][1] = x*y*(1.-c) - z*s;
    res[0][2] = x*z*(1.-c) + y*s;
    res[1][0] = y*x*(1.-c) + z*s;
    res[1][1] = y*y*(1.-c) + c;
    res[1][2] = y*z*(1.-c) - x*s;
    res[2][0] = z*x*(1.-c) - y*s;
    res[2][1] = z*y*(1.-c) + x*s;
    res[2][2] = z*z*(1.-c) + c;
}

void mat3_from_euler_angles(Mat3 res, double alpha, double beta, double gamma) {
    // see https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
    double c1 = cos(alpha), s1 = sin(alpha);
    double c2 = cos(beta), s2 = sin(beta);
    double c3 = cos(gamma), s3 = sin(gamma);
    res[0][0] = c1*c3-c2*s1*s3;
    res[0][1] = -c1*s3-c2*c3*s1;
    res[0][2] = s1*s2;
    res[1][0] = c3*s1+c1*c2*s3;
    res[1][1] = c1*c2*c3-s1*s3;
    res[1][2] = -c1*s2;
    res[2][0] = s2*s3;
    res[2][1] = c3*s2;
    res[2][2] = c2;
}
