#ifndef GLM_HPP
#define GLM_HPP

#include <glm/glm.hpp>
#include <glm/ext/quaternion_double.hpp>

static const double HACK_TO_KEEP_GLM_FROM_WRAPING_QUATERNION = pow(2., 100.);

// Fix for loss of precision of GLM with quaternions of small angles
template<typename T>
T angle(glm::qua<T> const& x) {
    constexpr T threshold = 0.8775825618903728;  // glm::cos(static_cast<T>(.5));
    if (x.w > threshold) {
        return asin(sqrt(x.x * x.x + x.y * x.y + x.z * x.z)) * static_cast<T>(2);
    } else {
        return acos(x.w) * static_cast<T>(2);
    }
}
template<typename T>
glm::qua<T> pow(glm::qua<T> const& x, T y) {
    //Raising to the power of 0 should yield 1
    //Needed to prevent a division by 0 error later on
    if(y > -glm::epsilon<T>() && y < glm::epsilon<T>())
        return glm::qua<T>(1,0,0,0);

    //To deal with non-unit quaternions
    T magnitude = sqrt(x.x * x.x + x.y * x.y + x.z * x.z + x.w *x.w);

    constexpr T threshold = 0.8775825618903728;  // glm::cos(static_cast<T>(.5));
    if (glm::abs(x.w / magnitude) > threshold) {
        //Scalar component is close to 1; using it to recover angle would lose precision
        //Instead, we use the non-scalar components since sin() is accurate around 0

        //Prevent a division by 0 error later on
        T VectorMagnitude = x.x * x.x + x.y * x.y + x.z * x.z;
        if (VectorMagnitude == 0) {
            //Equivalent to raising a real number to a power
            return glm::qua<T>(glm::pow(x.w, y), 0, 0, 0);
        }

        T SinAngle = sqrt(VectorMagnitude) / magnitude;
        T Angle = asin(SinAngle);
        T NewAngle = Angle * y;
        T Div = sin(NewAngle) / SinAngle;
        T Mag = glm::pow(magnitude, y - static_cast<T>(1));
        return glm::qua<T>(cos(NewAngle) * magnitude * Mag, x.x * Div * Mag, x.y * Div * Mag, x.z * Div * Mag);
    } else {
        //Scalar component is small, shouldn't cause loss of precision
        T Angle = acos(x.w / magnitude);
        T NewAngle = Angle * y;
        T Div = sin(NewAngle) / sin(Angle);
        T Mag = glm::pow(magnitude, y - static_cast<T>(1));
        return glm::qua<T>(cos(NewAngle) * magnitude * Mag, x.x * Div * Mag, x.y * Div * Mag, x.z * Div * Mag);
    }
}

#endif
