#!/usr/bin/env python3
import json
from datetime import datetime
from math import acos, asin, atan, cos, pi, sin, tan
from typing import Dict, Tuple

from parse import (
    ReferencePlane, parse_factsheets, parse_p_elem_t1, parse_planet_phys_par,
    parse_sat_elem, parse_sat_phys_par, parse_sbdb, planet_phys_par_G,
)

J2000 = datetime(2000, 1, 1, 12)
obliquity_of_the_ecliptic = 0.40910517666747087


def get_sun_physics(bodies: Dict) -> None:
    """Get physical information of the Sun"""

    bodies['Sun'] = {
        'gravitational_parameter': 1.3271244018e20,
        'radius': 6.96e8,
        'rotational_period': 2192832.0,
    }


def get_planets_physics(bodies: Dict) -> None:
    """Get physical information of planets of the Solar System"""
    G = planet_phys_par_G()

    for planet in parse_planet_phys_par():
        bodies[planet.name] = {
            'gravitational_parameter': G * planet.mass,
            'radius': planet.mean_radius,
            'rotational_period': planet.sidereal_rotation_period,
        }


def get_more_physics(bodies: Dict) -> None:
    """Get more physical information on a body of the Solar System"""
    for body in parse_factsheets():
        if body.north_pole_right_ascension is None:
            continue
        assert body.north_pole_declination is not None

        if bodies[body.name]['rotational_period'] < 0 and body.name != 'Uranus':  # TODO
            print(body.name, bodies[body.name]['rotational_period'])
            # we're going to guess the positive pole is opposite to the north pole
            bodies[body.name]['positive_pole'] = {
                'right_ascension': body.north_pole_right_ascension + pi,
                'declination': -body.north_pole_declination,
            }
        else:
            bodies[body.name]['positive_pole'] = {
                'right_ascension': body.north_pole_right_ascension,
                'declination': body.north_pole_declination,
            }


def get_planets_orbits(bodies: Dict) -> None:
    """Get orbital information of planets of the Solar System"""
    for orbit in parse_p_elem_t1():
        name = orbit.name
        if name == 'EM Bary':
            name = 'Earth'

        # TODO: apply linear rate
        semi_major_axis = orbit.semi_major_axis[0]
        eccentricity = orbit.eccentricity[0]
        inclination = orbit.inclination[0]
        mean_longitude = orbit.mean_longitude[0]
        longitude_of_periapsis = orbit.longitude_of_periapsis[0]
        longitude_of_ascending_node = orbit.longitude_of_ascending_node[0]

        # deduce back mean anomaly and argument of periapsis
        mean_anomaly = mean_longitude - longitude_of_periapsis
        argument_of_periapsis = longitude_of_periapsis - longitude_of_ascending_node

        body = bodies.setdefault(name, {})
        body['orbit'] = {
            'primary': 'Sun',
            'semi_major_axis': semi_major_axis,
            'eccentricity': eccentricity,
            'inclination': inclination,
            'longitude_of_ascending_node': orbit.longitude_of_ascending_node[0],
            'argument_of_periapsis': argument_of_periapsis,
            'epoch': 0,
            'mean_anomaly_at_epoch': mean_anomaly,
        }


def get_moons_physics(bodies: Dict) -> None:
    """Get physical information of moons of the Solar System"""
    for moon in parse_sat_phys_par():
        bodies[moon.name] = {
            'gravitational_parameter': moon.gravitational_parameter,
            'radius': moon.mean_radius,
        }


def emo_from_eme(right_ascension: float, declination: float) -> Tuple[float, float]:
    # see https://en.wikipedia.org/wiki/Celestial_coordinate_system#Equatorial_%E2%86%94_ecliptic
    ε = obliquity_of_the_ecliptic
    α = right_ascension
    δ = declination
    λ = atan((sin(α) * cos(ε) + tan(δ) * sin(ε)) / cos(α))
    β = asin(sin(δ) * cos(ε) - cos(δ) * sin(ε) * sin(α))
    ecliptic_longitude = λ
    ecliptic_latitude = β
    return ecliptic_longitude, ecliptic_latitude


def get_moons_orbits(bodies: Dict) -> None:
    """Get orbital information of moons of the Solar System"""

    for orbit in parse_sat_elem():
        if orbit.reference_plane == ReferencePlane.ECLIPTIC:
            inclination = orbit.inclination
            longitude_of_ascending_node = orbit.longitude_of_ascending_node
        else:
            # when given equatorial elements, convert to ecliptic elements
            # when given Laplacian elements, handle as equatorial elements

            # inclination is given relative to the equatorial plane of
            # the primary; longitude of the ascending node is given
            # relative to the northward equinox

            # north pole of the primary
            positive_pole = bodies[orbit.primary]['positive_pole']
            positive_pole_ecliptic_longitude, positive_pole_ecliptic_latitude = emo_from_eme(
                right_ascension=positive_pole['right_ascension'],
                declination=positive_pole['declination'],
            )

            # from http://www.krysstal.com/sphertrig.html
            # Setting:
            # - blue great circle as the ecliptic
            # - point A as the normal of the ecliptic
            # - point B as the primary's north pole
            # - point C as the normal of the satellite's orbital plane
            #
            # Note that:
            # - B' is orthogonal to the line of nodes of the primary
            # - C' is orthogonal to the line of nodes of the satellite
            #
            # Then:
            # - A
            # - B is π-complementary to the longitude of the ascending node in the primary's equator plane
            # - C
            # - a is the orbital inclination in the primary's equator plane
            # - b is the orbital inclination in the ecliptic
            # - c is π/2-complementary to the ecliptic latitude of the primary's north pole

            # set known values
            a = orbit.inclination
            c = pi / 2 - positive_pole_ecliptic_latitude
            B = pi - orbit.longitude_of_ascending_node

            # From the spherical law of cosines, we get:
            b = acos(cos(a) * cos(c) + sin(a) * sin(c) * cos(B))

            # From the spherical law of sines, we get:
            A = asin(sin(B) * sin(a) / sin(b))

            if orbit.name == 'Charon':
                from math import degrees
                print(degrees(positive_pole['right_ascension']))
                print(degrees(positive_pole['declination']))
                print(degrees(positive_pole_ecliptic_longitude))
                print(degrees(positive_pole_ecliptic_latitude))
                print()

            # ecliptic elements
            inclination = b
            longitude_of_orbital_normal = positive_pole_ecliptic_longitude + A
            if orbit.reference_plane == ReferencePlane.LAPLACE:
                # TODO: this is an approximation
                longitude_of_ascending_node = longitude_of_orbital_normal + pi / 2
            else:
                longitude_of_ascending_node = longitude_of_orbital_normal - pi / 2

        # save orbit
        body = bodies.setdefault(orbit.name, {})
        body['orbit'] = {
            'primary': orbit.primary,
            'semi_major_axis': orbit.semi_major_axis,
            'eccentricity': orbit.eccentricity,
            'inclination': inclination,
            'longitude_of_ascending_node': longitude_of_ascending_node,
            'argument_of_periapsis': orbit.argument_of_periapsis,
            'epoch': (orbit.epoch - J2000).total_seconds(),
            'mean_anomaly_at_epoch': orbit.mean_anomaly_at_epoch,
        }


def get_dwarf_planet_data(bodies: Dict, name: str) -> None:
    """Get physical and orbital information of dwarf planets"""
    data = parse_sbdb(name)

    body = bodies.setdefault(name, {})

    if data.radius is not None:
        body['radius'] = data.radius
    if data.gravitational_parameter is not None:
        body['gravitational_parameter'] = data.gravitational_parameter
    if data.rotation_period is not None:
        body['rotational_period'] = data.rotation_period

    body['orbit'] = {
        'primary': 'Sun',
        'semi_major_axis': data.semi_major_axis,
        'eccentricity': data.eccentricity,
        'inclination': data.inclination,
        'longitude_of_ascending_node': data.longitude_of_ascending_node,
        'argument_of_periapsis': data.argument_of_periapsis,
        'epoch': (data.epoch - J2000).total_seconds(),
        'mean_anomaly_at_epoch': data.mean_anomaly_at_epoch,
    }


def main() -> None:
    bodies: Dict = {}

    # Sun
    get_sun_physics(bodies)

    # physics
    get_planets_physics(bodies)
    get_moons_physics(bodies)
    get_more_physics(bodies)

    # orbits
    get_planets_orbits(bodies)
    get_moons_orbits(bodies)

    # dwarf planets
    dwarf_planets = ('Ceres', 'Pluto', 'Sedna', 'Haumea', 'Makemake', 'Eris')
    for dwarf_planet in dwarf_planets:
        get_dwarf_planet_data(bodies, dwarf_planet)

    # dwarf planets radii
    # TODO: need other source for this
    bodies['Ceres']['radius'] = 473e3
    bodies['Pluto']['radius'] = 1188.3e3
    bodies['Sedna']['radius'] = 500e3
    bodies['Haumea']['radius'] = 798e3
    bodies['Makemake']['radius'] = 715e3
    bodies['Eris']['radius'] = 1163e3

    with open('../data/solar_system.json', 'w') as f:
        json.dump(bodies, f, sort_keys=True, indent=4, separators=(',', ': '))
        f.write('\n')


if __name__ == '__main__':
    main()
