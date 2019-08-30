#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <algorithm>
#include <cmath>

template<class S, size_t N>
struct vector {
    using V = vector<S, N>;

    S coords[N];

    S& operator[](size_t i) {
        return this->coords[i];
    }

    S operator[](size_t i) const {
        return this->coords[i];
    }

    template <class O>
    friend O& operator<<(O& lhs, const V& rhs) {
        lhs << "{" << rhs[0];
        for (size_t i = 1; i < N; i += 1) {
            lhs << ", " << rhs[i];
        }
        lhs << "}";
        return lhs;
    }

    #define UNARY_OP(OP) \
    V operator OP() const & { \
        V ret; \
        for (size_t i = 0; i < N; i += 1) { \
            ret[i] = OP (*this)[i]; \
        } \
        return ret; \
    }
    UNARY_OP(+)
    UNARY_OP(-)
    #undef UNARY_OP

    #define SCALAR_INPLACE_OP(OP) \
    V& operator OP(S rhs) & { \
        for (size_t i = 0; i < N; i += 1) { \
            (*this)[i] OP rhs; \
        } \
        return *this; \
    }
    SCALAR_INPLACE_OP(+=)
    SCALAR_INPLACE_OP(-=)
    SCALAR_INPLACE_OP(*=)
    SCALAR_INPLACE_OP(/=)
    SCALAR_INPLACE_OP(%=)
    #undef SCALAR_INPLACE_OP

    #define SCALAR_OP(OP) \
    V operator OP(S rhs) const { \
        const V& lhs = *this; \
        V ret; \
        for (size_t i = 0; i < N; i += 1) { \
            ret[i] = lhs[i] OP rhs; \
        } \
        return ret; \
    } \
    friend V operator OP(S lhs, const V& rhs) { \
        V ret; \
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
    #define VECTOR_INPLACE_OP(OP) \
    V& operator OP(const V& rhs) & { \
        for (size_t i = 0; i < N; i += 1) { \
            (*this)[i] OP rhs[i]; \
        } \
        return *this; \
    }
    VECTOR_INPLACE_OP(+=)
    VECTOR_INPLACE_OP(-=)
    #undef INPLACE_OP

    #define VECTOR_OP(OP) \
    V operator OP(const V& rhs) const { \
        const V& lhs = *this; \
        V ret; \
        for (size_t i = 0; i < N; i += 1) { \
            ret[i] = lhs[i] OP rhs[i]; \
        } \
        return ret; \
    }
    VECTOR_OP(+)
    VECTOR_OP(-)
    #undef VECTOR_OP

    S dot(const V& rhs) const {
        const V& lhs = *this;
        S ret = 0;
        for (size_t i = 0; i < N; i += 1) {
            ret += lhs[i] * rhs[i];
        }
        return ret;
    }

    S unorm() const {
        const V& self = *this;
        S ret = 0;
        for (size_t i = 0; i < N; i += 1) {
            S v = std::abs(self[i]);
            if (v > ret) {
                ret = v;
            }
        }
        return ret;
    }

    S norm() const {
        const V& lhs = *this;
        return std::sqrt(lhs.dot(lhs));
    }

    S udist(const V& rhs) const {
        const V& lhs = *this;
        return (lhs - rhs).unorm();
    }

    S dist(const V& rhs) const {
        const V& lhs = *this;
        return (lhs - rhs).norm();
    }

    S angle(const V& rhs) const {
        const V& lhs = *this;
        // NOTE: can save a call to std::sqrt
        S r = lhs.dot(rhs) / lhs.norm() / rhs.norm();
        // clip to -1..1
        r = std::max<S>(-1, std::min<S>(r, 1));
        return std::acos(r);
    }

    S angle2(const V& rhs, const V& normal=V{0, 0, 1}) {
        const V& lhs = *this;
        S geometric_angle = lhs.angle(rhs);
        if (normal.dot(lhs.cross(rhs)) < 0.) {
            return -geometric_angle;
        } else {
            return geometric_angle;
        }
    }

    // only defined for N = 3 and N = 7
    V cross(const V& rhs) const {
        const V& lhs = *this;
        return cross_impl(lhs, rhs);
    }
};

// https://en.wikipedia.org/wiki/Cross_product
template<class S>
vector<S, 3> cross_impl(const vector<S, 3>& lhs, const vector<S, 3>& rhs) {
    return {
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    };
}

// https://en.wikipedia.org/wiki/Seven-dimensional_cross_product
template<class S>
vector<S, 7> cross_impl(const vector<S, 7>& lhs, const vector<S, 7>& rhs) {
    return {
        lhs[1] * rhs[3] - lhs[3] * rhs[1] + lhs[2] * rhs[6] - lhs[6] * rhs[2] + lhs[4] * rhs[5] - lhs[5] * rhs[4],
        lhs[2] * rhs[4] - lhs[4] * rhs[2] + lhs[3] * rhs[0] - lhs[0] * rhs[3] + lhs[5] * rhs[6] - lhs[6] * rhs[5],
        lhs[3] * rhs[5] - lhs[5] * rhs[3] + lhs[4] * rhs[1] - lhs[1] * rhs[4] + lhs[6] * rhs[0] - lhs[0] * rhs[6],
        lhs[4] * rhs[6] - lhs[6] * rhs[4] + lhs[5] * rhs[2] - lhs[2] * rhs[5] + lhs[0] * rhs[1] - lhs[1] * rhs[0],
        lhs[5] * rhs[0] - lhs[0] * rhs[5] + lhs[6] * rhs[3] - lhs[3] * rhs[6] + lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[6] * rhs[1] - lhs[1] * rhs[6] + lhs[0] * rhs[4] - lhs[4] * rhs[0] + lhs[2] * rhs[3] - lhs[3] * rhs[2],
        lhs[0] * rhs[2] - lhs[2] * rhs[0] + lhs[1] * rhs[5] - lhs[5] * rhs[1] + lhs[3] * rhs[4] - lhs[4] * rhs[3],
    };
}

using vec3 = vector<double, 3>;
using vec6 = vector<double, 6>;

#endif
