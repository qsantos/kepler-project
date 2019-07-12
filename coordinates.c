#include "coordinates.h"

#include <math.h>

void coordinates_from_equatorial(CelestialCoordinates* c, double right_ascension, double declination, double distance) {
    c->right_ascension = right_ascension;
    c->declination = declination;
    c->distance = distance;

    // compute equivalent coordinates in ecliptic plane
    double e = obliquity_of_the_ecliptic;
    c->ecliptic_longitude = atan2(
            cos(declination)*sin(right_ascension)*cos(e) + sin(declination)*sin(e),
            cos(declination)*cos(right_ascension)
    );
    c->ecliptic_latitude = asin(sin(declination) * cos(e) - cos(declination) * sin(e) * sin(right_ascension));
}

void coordinates_from_ecliptic(CelestialCoordinates* c, double ecliptic_longitude, double ecliptic_latitude, double distance) {
    c->ecliptic_longitude = ecliptic_longitude;
    c->ecliptic_latitude = ecliptic_latitude;
    c->distance = distance;

    // compute equivalent coordinates in equatorial plane
    double e = obliquity_of_the_ecliptic;
    c->right_ascension = atan2(
            cos(ecliptic_latitude)*sin(ecliptic_longitude)*cos(e) - sin(ecliptic_latitude)*sin(e),
            cos(ecliptic_latitude)*cos(ecliptic_longitude)
    );
    c->declination = asin(sin(ecliptic_latitude) * cos(e) + cos(ecliptic_latitude) * sin(e) * sin(ecliptic_longitude));
}
