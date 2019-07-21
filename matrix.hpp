#ifndef MATRIX_HPP
#define MATRIX_HPP

#include "vector.hpp"

#include <cassert>

template<class S, size_t N, size_t M>
struct matrix;

template<class S>
static matrix<S, 3, 3> from_angle_axis_impl(S angle, S x, S y, S z) {
    S s = std::sin(angle);
    S c = std::cos(angle);
    S d = std::sqrt(x*x + y*y + z*z);
    x /= d;
    y /= d;
    z /= d;
    return {
        {x*x*(1.-c) + c,   x*y*(1.-c) - z*s, x*z*(1.-c) + y*s},
        {y*x*(1.-c) + z*s, y*y*(1.-c) + c,   y*z*(1.-c) - x*s},
        {z*x*(1.-c) - y*s, z*y*(1.-c) + x*s, z*z*(1.-c) + c  },
    };
}

template<class S>
static matrix <S, 3, 3> from_euler_angles_impl(S alpha, S beta, S gamma) {
    // see https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
    S c1 = std::cos(alpha), s1 = std::sin(alpha);
    S c2 = std::cos(beta),  s2 = std::sin(beta);
    S c3 = std::cos(gamma), s3 = std::sin(gamma);
    return {
        {c1*c3-c2*s1*s3, -c1*s3-c2*c3*s1,  s1*s2},
        {c3*s1+c1*c2*s3,  c1*c2*c3-s1*s3, -c1*s2},
        {s2*s3,           c3*s2,           c2   },
    };
}


template<class S, size_t N, size_t M>
struct matrix{
    using COLUMN = vector<S, N>;
    using ROW = vector<S, M>;
    using W = matrix<S, N, M>;

    ROW rows[N];

    matrix() {
    }

    matrix(std::initializer_list<std::initializer_list<S>> values) {
        assert(values.size() == N);
        size_t i = 0;
        for (auto& row: values) {
            assert(row.size() == M);
            size_t j = 0;
            for (auto& value: row) {
                (*this)[i][j] = value;
                j += 1;
            }
            i += 1;
        }
    }

    ROW& operator[](size_t i) {
        return this->rows[i];
    }

    ROW operator[](size_t i) const {
        return this->rows[i];
    }

    template <class O>
        friend O& operator<<(O& lhs, const W& rhs) {
            lhs << "{" << "\n";
            for (size_t i = 0; i < N; i += 1) {
                lhs << "\t" << rhs[i] << ",\n";
            }
            lhs << "}";
            return lhs;
        }

#define UNARY_OP(OP) \
    W operator OP() const { \
        const W& self = *this; \
        W ret; \
        for (size_t i = 0; i < N; i += 1) { \
            ret[i] = OP self[i]; \
        } \
        return ret; \
    }
    UNARY_OP(+)
        UNARY_OP(-)
#undef UNARY_OP

#define SCALAR_OP(OP) \
        W operator OP(S rhs) const { \
        const W& lhs = *this; \
        W ret; \
        for (size_t i = 0; i < N; i += 1) { \
            ret[i] = lhs[i] OP rhs; \
        } \
        return ret; \
    } \
    friend W operator OP(S lhs, const W& rhs) { \
        W ret; \
        for (size_t i = 0; i < N; i += 1) { \
            ret[i] = lhs OP rhs[i]; \
        } \
        return ret; \
    }
    SCALAR_OP(+)
    SCALAR_OP(-)
    SCALAR_OP(*)
    SCALAR_OP(/)
    SCALAR_OP(%)
    #undef SCALAR_OP

    // cppcheck does not like OP##=
    #define WECTOR_INPLACE_OP(OP) \
    W& operator OP(const W& rhs) { \
        const W& lhs = *this; \
        for (size_t i = 0; i < N; i += 1) { \
            lhs[i] OP rhs[i]; \
        } \
        return *this; \
    }
    WECTOR_INPLACE_OP(+=)
    WECTOR_INPLACE_OP(-=)
    #undef INPLACE_OP

    #define WECTOR_OP(OP) \
    W operator OP(const W& rhs) const { \
        const W& lhs = *this; \
        W ret; \
        for (size_t i = 0; i < N; i += 1) { \
            ret[i] = lhs[i] OP rhs[i]; \
        } \
        return ret; \
    }
    WECTOR_OP(+)
    WECTOR_OP(-)
    #undef WECTOR_OP

    S unorm() const {
        const W& self = *this;
        S ret = 0;
        for (size_t i = 0; i < N; i += 1) {
            S v = self[i].norm();
            if (v > ret) {
                ret = v;
            }
        }
        return ret;
    }

    S udist(W rhs) {
        const W& lhs = *this;
        return (lhs - rhs).unorm();
    }

    static W from_angle_axis(S angle, S x, S y, S z) {
        return from_angle_axis_impl(angle, x, y, z);
    }

    static W from_euler_angles(S alpha, S beta, S gamma) {
        return from_euler_angles_impl(alpha, beta, gamma);
    }

};

template<class S, size_t N, size_t M, size_t K>
matrix<S, N, M> operator*(matrix<S, N, K> lhs, matrix<S, K, M> rhs) {
    matrix<S, N, M> ret;
    for (size_t i = 0; i < N; i += 1) {
        for (size_t j = 0; j < M; j += 1) {
            S sum = 0;
            for (size_t k = 0; k < K; k += 1) {
                sum += lhs[i][k] * rhs[k][j];
            }
            ret[i][j] = sum;
        }
    }
    return ret;
}

template<class S, size_t N, size_t M>
vector<S, M> operator*(matrix<S, N, M> lhs, vector<S, N> rhs) {
    vector<S, N> ret;
    for (size_t i = 0; i < N; i += 1) {
        ret[i] = lhs[i].dot(rhs);
    }
    return ret;
}

using mat3 = matrix<double, 3, 3>;

#endif
