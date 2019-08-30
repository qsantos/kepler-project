#ifndef COORDINATES_HPP
#define COORDINATES_HPP

#include <glm/glm.hpp>
#include <cmath>

// obliquity of the ecliptic = Earth's tilt
static const double obliquity_of_the_ecliptic = 0.40910517666747087;

struct CelestialCoordinates {
    // equatorial coordinates
    double right_ascension;
    double declination;
    // ecliptic coordinates
    double ecliptic_longitude;
    double ecliptic_latitude;
    // infinite for a direction, finite for a position
    double distance;

    static CelestialCoordinates from_equatorial(double right_ascension, double declination, double distance) {
        double e = obliquity_of_the_ecliptic;
        return {
            right_ascension,
            declination,
            atan2(
                    cos(declination)*sin(right_ascension)*cos(e) + sin(declination)*sin(e),
                    cos(declination)*cos(right_ascension)
            ),
            asin(sin(declination) * cos(e) - cos(declination) * sin(e) * sin(right_ascension)),
            distance,
        };
    }

    static CelestialCoordinates from_ecliptic(double ecliptic_longitude, double ecliptic_latitude, double distance) {
        double e = obliquity_of_the_ecliptic;

        return {
            atan2(
                    cos(ecliptic_latitude)*sin(ecliptic_longitude)*cos(e) - sin(ecliptic_latitude)*sin(e),
                    cos(ecliptic_latitude)*cos(ecliptic_longitude)
            ),
            asin(sin(ecliptic_latitude) * cos(e) + cos(ecliptic_latitude) * sin(e) * sin(ecliptic_longitude)),
            ecliptic_longitude,
            ecliptic_latitude,
            distance,
        };
    }

    static CelestialCoordinates from_cartesian(double x, double y, double z) {
        double r = sqrt(x * x + y * y + z * z);
        return CelestialCoordinates::from_ecliptic(atan2(y, x), acos(z / r), r);
    }

    static CelestialCoordinates from_cartesian(glm::dvec3 pos) {
        return CelestialCoordinates::from_cartesian(pos[0], pos[1], pos[2]);
    }
};

#endif
