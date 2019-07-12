#ifndef VECTOR_H
#define VECTOR_H

typedef double Vec3[3];
typedef Vec3 Mat3[3];

double vec3_unorm (const Vec3 u);
double vec3_norm  (const Vec3 u);
double vec3_udist (const Vec3 u, const Vec3 v);
double vec3_dist  (const Vec3 u, const Vec3 v);
double vec3_dot   (const Vec3 u, const Vec3 v);
double vec3_angle (const Vec3 u, const Vec3 v);
double vec3_angle2(const Vec3 u, const Vec3 v, const Vec3 normal);

void vec3_add  (Vec3 res, const Vec3 u, const Vec3 v);
void vec3_sub  (Vec3 res, const Vec3 u, const Vec3 v);
void vec3_scale(Vec3 res, const Vec3 u, double s);
void vec3_cross(Vec3 res, const Vec3 u, const Vec3 v);

double mat3_unorm(Mat3 a);
double mat3_udist(Mat3 a, Mat3 b);

void mat3_add (Mat3 res, Mat3 a, Mat3 b);
void mat3_sub (Mat3 res, Mat3 a, Mat3 b);
void mat3_mulv(Vec3 res, Mat3 a, Vec3 b);
void mat3_mulm(Mat3 res, Mat3 a, Mat3 b);

void mat3_from_angle_axis(Mat3 res, double angle, double x, double y, double z);
void mat3_from_euler_angles(Mat3 res, double alpha, double beta, double gamma);

#endif
