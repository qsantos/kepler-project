#ifndef COORDINATES_H
#define COORDINATES_H

typedef struct celestial_coordinates CelestialCoordinates;

// obliquity of the ecliptic = Earth's tilt
static const double obliquity_of_the_ecliptic = 0.40910517666747087;

struct celestial_coordinates {
    // equatorial coordinates
    double right_ascension;
    double declination;
    // ecliptic coordinates
    double ecliptic_longitude;
    double ecliptic_latitude;
    // infinite for a direction, finite for a position
    double distance;
};

void coordinates_from_equatorial(CelestialCoordinates* c, double right_ascension, double declination, double distance);
void coordinates_from_ecliptic  (CelestialCoordinates* c, double ecliptic_longitude, double ecliptic_latitude, double distance);

#endif
