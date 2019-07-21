#include "lambert.hpp"

#include <cassert>

extern "C" {
#include "util.h"
}

// from "Revisiting Lambert’s Problem" by Dario Izzo
// see <https://github.com/esa/pykep/blob/master/src/core_functions/lambert_3d.h>
// see <https://github.com/esa/pykep/blob/master/src/core_functions/lambert_2d.h>
// see also <https://github.com/poliastro/poliastro/blob/master/src/poliastro/iod/izzo.py>
// notations try to match that of poliastro or of the paper

static double _compute_y(double lambda, double x) {
    return sqrt(1. - lambda*lambda*(1. - x*x));
}

static double _compute_psi(double lambda, double x, double y) {
    if (-1. <= x && x <= 1.) {
        return acos(x*y + lambda*(1. - x*x));
    } else if (x > 1.) {
        return asinh((y - x*lambda) * sqrt(x*x - 1));
    } else {
        return 0.;
    }
}

// inspired from from <https://github.com/poliastro/poliastro/blob/master/src/poliastro/hyper.py>
static double _hyp2f1b(double x) {
    // Hypergeometric function 2F1(3, 1, 5/2, x), see [Battin].
    if (x >= 1.) {
        return INFINITY;
    }
    // TODO: Kahan summation algorithm might be worth it here
    double acc = 1.;
    double term = 1.;
    double i = 0.;
    while (1) {
        term *= (3. + i) / (2.5 + i) * x;
        double new_acc = acc + term;
        if (new_acc == acc) {
            return acc;
        }
        acc = new_acc;
        i += 1.;
    }
}

static double _compute_x0(double lambda, double T, int M, int right_branch) {
    if (M == 0) {
        // compute x_0 from Eq.(30)
        double T_00 = acos(lambda) + lambda * sqrt(1 - lambda*lambda);
        double T_0 = T_00 + M*M_PI;
        double T_1 = 2./3. * (1. - lambda*lambda*lambda);
        if (T < T_1) {
            return 2.5 * T_1 * (T_1 - T) / (T * (1 - lambda*lambda*lambda*lambda*lambda)) + 1.;
        } else if (T < T_0) {
            return pow(T_0 / T, log(T_1 / T_0) / log(2.)) - 1.;
        } else {
            return pow(T_0 / T, 2./3.) - 1.;
        }
    } else if (right_branch) {  // right branch
        // compute x_0r from Eq.(31) with M = M_max
        double t = pow((8. * T) / (M * M_PI), 2./3.);
        return (t - 1.) / (t + 1.);
    } else {  // left branch
        // compute x_0l from Eq.(31) with M = M_max
        double t = pow((M * M_PI + M_PI) / (8. * T), 2./3.);
        return (t - 1.) / (t + 1.);
    }
}

static double _householder(double lambda, double Tstar, double M, double x_0) {
    double x = x_0;
    for (int i = 0; i < 35; i += 1) {
        double y = _compute_y(lambda, x);
        double psi = _compute_psi(lambda, x, y);
        // compute T(x) and derivatives
        double T = ((psi + M*M_PI) / sqrt(fabs(1. - x*x)) - x + lambda*y) / (1. - x*x);
        if (M == 0. && sqrt(.6) < x && x < sqrt(1.4)) {
            double eta = y - lambda * x;
            double S_1 = (1. - lambda - x *eta) * .5;
            double Q = 4. / 3.* _hyp2f1b(S_1);
            T = (eta*eta*eta*Q + 4.*lambda*eta) * .5;
        }
        double Tp = (3.*T*x - 2. + 2.*lambda*lambda*lambda*x/y) / (1. - x*x);
        double Tpp = (3.*T + 5.*x*Tp + 2.*(1. - lambda*lambda)*lambda*lambda*lambda/(y*y*y)) / (1. - x*x);
        double Tppp = (7.*x*Tpp + 8.*Tp - 6.*(1. - lambda*lambda)*lambda*lambda*lambda*lambda*lambda*x/(y*y*y*y*y)) / (1. - x*x);
        // f = T - T^*
        double f = T - Tstar;
        double fp = Tp;
        double fpp = Tpp;
        double fppp = Tppp;
        double new_x = x - f * (fp*fp - f*fpp/2.) / (fp*(fp*fp - f*fpp) + fppp*f*f/6.);
        if (new_x == x) {  // paper advises for 1e-5 or 1e-9 absolute error
            break;
        }
        x = new_x;
    }
    return x;
}

void lambert(Vec3 v1, Vec3 v2, double mu, Vec3 r1, Vec3 r2, double t, int M, int right_branch) {
    // TODO: compute M_max and check M

    // some scalars
    Vec3 c;
    vec3_sub(c, r2, r1);
    double r1_norm = vec3_norm(r1);
    double r2_norm = vec3_norm(r2);
    double c_norm = vec3_norm(c);
    double s = .5 * (r1_norm + r2_norm + c_norm);
    double lambda = sqrt(1. - c_norm/s);

    // various unit vectors
    Vec3 i_r1, i_r2, i_t1, i_t2, i_h;
    vec3_scale(i_r1, r1, 1./r1_norm);
    vec3_scale(i_r2, r2, 1./r2_norm);
    vec3_cross(i_h, i_r1, i_r2);
    vec3_scale(i_h, i_h, 1. / vec3_norm(i_h));  // <https://github.com/poliastro/poliastro/blob/master/src/poliastro/iod/izzo.py#L67>
    // <https://github.com/poliastro/poliastro/blob/master/src/poliastro/iod/izzo.py#L72-L76>
    if (i_h[2] < 0.) {
        lambda = -lambda;
        vec3_scale(i_h, i_h, -1.);
    }
    vec3_cross(i_t1, i_h, i_r1);
    vec3_cross(i_t2, i_h, i_r2);

    // make t unitless
    double T = t * sqrt(2.*mu / (s*s*s));

    // find x and y
    double x_0 = _compute_x0(lambda, T, M, right_branch);
    double x = _householder(lambda, T, M, x_0);
    double y = _compute_y(lambda, x);

    // velocity components
    double gamma = sqrt(mu * s / 2.);
    double rho = (r1_norm - r2_norm) / c_norm;
    double sigma = sqrt(1. - rho*rho);
    double V_r1 =  gamma * ((lambda*y - x) - rho * (lambda*y + x)) / r1_norm;
    double V_r2 = -gamma * ((lambda*y - x) + rho * (lambda*y + x)) / r2_norm;
    double V_t1 = gamma * sigma * (y + lambda*x) / r1_norm;
    double V_t2 = gamma * sigma * (y + lambda*x) / r2_norm;

    // compute velocity vectors
    Vec3 tmp1, tmp2;
    // v1 = V_r1 * i_r1 + V_t1 * i_t1
    vec3_scale(tmp1, i_r1, V_r1);
    vec3_scale(tmp2, i_t1, V_t1);
    vec3_add(v1, tmp1, tmp2);
    // v2 = V_r2 * i_r2 + V_t2 * i_t2
    vec3_scale(tmp1, i_r2, V_r2);
    vec3_scale(tmp2, i_t2, V_t2);
    vec3_add(v2, tmp1, tmp2);
}
