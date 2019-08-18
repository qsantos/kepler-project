#!/usr/bin/env python3
import re
from datetime import datetime, timedelta
from enum import Enum
from math import radians
from typing import Any, Dict, Iterator, List, NamedTuple, Optional, Tuple

astronomical_unit = 149597870700.
century = 36525 * 86400


class PlanetPhysParDataRow(NamedTuple):
    name: str
    equatorial_radius: float
    mean_radius: float
    mass: float
    bulk_density: float
    sidereal_rotation_period: float
    sidereal_orbital_period: float
    magnitude: float
    geometric_albedo: float
    equatorial_gravity: float
    escape_velocity: float


class FactsheetDataRow(NamedTuple):
    name: str
    # TODO: many other values are available
    # TODO: precession
    # north pole of rotation
    north_pole_right_ascension: Optional[float]
    north_pole_declination: Optional[float]


class PElemDataRow(NamedTuple):
    name: str
    semi_major_axis: Tuple[float, float]
    eccentricity: Tuple[float, float]
    inclination: Tuple[float, float]
    mean_longitude: Tuple[float, float]
    longitude_of_periapsis: Tuple[float, float]
    longitude_of_ascending_node: Tuple[float, float]


class SatPhysParDataRow(NamedTuple):
    name: str
    primary: str
    gravitational_parameter: float
    mean_radius: float
    mean_density: Optional[float]
    magnitude: float
    geometric_albedo: Optional[float]


class ReferencePlane(str, Enum):
    ECLIPTIC = 'ECLIPTIC'
    EQUATORIAL = 'EQUATORIAL'
    LAPLACE = 'LAPLACE'


class SatElemDataRow(NamedTuple):
    name: str
    primary: str
    reference_plane: ReferencePlane
    epoch: datetime
    semi_major_axis: float
    eccentricity: float
    argument_of_periapsis: float
    mean_anomaly_at_epoch: float
    inclination: float
    longitude_of_ascending_node: float
    longitude_rate: float
    period: float
    argument_of_periapsis_precession_period: float
    longitude_of_ascending_node_precession_period: float
    north_pole_right_ascension: Optional[float]
    north_pole_declination: Optional[float]
    tilt: Optional[float]


class SBDBDataRow(NamedTuple):
    name: str
    primary: str
    epoch: datetime
    eccentricity: float
    semi_major_axis: float
    periapsis: float
    inclination: float
    longitude_of_ascending_node: float
    argument_of_periapsis: float
    mean_anomaly_at_epoch: float
    orbital_period: float
    mean_motion: float
    apoapsis: float

    absolute_magnitude: Optional[float] = None
    magnitude_slope: Optional[float] = None
    radius: Optional[float] = None
    gravitational_parameter: Optional[float] = None
    bulk_density: Optional[float] = None
    rotation_period: Optional[float] = None
    north_pole_right_ascension: Optional[float] = None
    north_pole_declination: Optional[float] = None
    geometric_albedo: Optional[float] = None


def planet_phys_par_G() -> float:
    with open('data/planet_phys_par.html') as f:
        html = f.read()

    match = re.search(r'(?s).*G=(6\.674[0-9]*)', html)
    assert match is not None
    return float(match.group(1)) * 1e-11


def parse_planet_phys_par() -> Iterator[PlanetPhysParDataRow]:
    with open('data/planet_phys_par.html') as f:
        html = f.read()

    # iterate over rows
    matches = re.findall(r'(?s)<td align="left">([^<]*)<br>&nbsp;</td>(.*?)</tr>', html)
    for planet, data in matches:
        # extract data cells from row (ingore precision information)
        matches = re.findall(r'<td align="right" nowrap>(.*)<br>', data)

        # unpack values
        (
            equatorial_radius, mean_radius, mass, bulk_density,
            sidereal_rotation_period, sidereal_orbital_period, magnitude, albedo,
            equatorial_gravity, escape_velocity,
        ) = [float(match) for match in matches]

        if planet == 'Pluto':
            # see IAU definition of positive pole for dwarf planets
            sidereal_rotation_period = abs(sidereal_rotation_period)

        # normalize to SI units and export
        yield PlanetPhysParDataRow(
            name=planet,
            equatorial_radius=equatorial_radius * 1e3,
            mean_radius=mean_radius * 1e3,
            mass=mass * 1e24,
            bulk_density=bulk_density * 1e3,
            sidereal_rotation_period=sidereal_rotation_period * 86400,
            sidereal_orbital_period=sidereal_orbital_period * 86400 * 365.25,
            magnitude=magnitude,
            geometric_albedo=albedo,
            equatorial_gravity=equatorial_radius,
            escape_velocity=escape_velocity * 1e3,
        )


def parse_factsheet(filename: str) -> FactsheetDataRow:
    with open(f'data/factsheet/{filename}') as f:
        html = f.read()

    match = re.search(r'<title>([^<]*) Fact Sheet</title>', html)
    assert match is not None
    name = match.group(1)

    # orientation of north pole (axial tilt)

    # extract right ascension
    match = re.search(r'Right Ascension *: *([\-0-9\.]+)', html)
    if match is None:
        north_pole_right_ascension = None
    else:
        north_pole_right_ascension = radians(float(match.group(1)))

    # extract declination
    match = re.search(r'Declination *: *([\-0-9\.]+)', html)
    if match is None:
        north_pole_declination = None
    else:
        north_pole_declination = radians(float(match.group(1)))

    return FactsheetDataRow(
        name=name,
        north_pole_right_ascension=north_pole_right_ascension,
        north_pole_declination=north_pole_declination,
    )


def parse_factsheets() -> Iterator[FactsheetDataRow]:
    with open(f'data/factsheet/index.html') as f:
        html = f.read()

    for filename in set(re.findall(r'href="([^"/]*fact.html)"', html)):
        yield parse_factsheet(filename)


def parse_p_elem(lines: List[str]) -> Iterator[PElemDataRow]:
    # iterate lines that represent rows in the table
    for i in range(0, len(lines), 2):
        # each row is made of two lines
        first_line = lines[i]
        second_line = lines[i + 1]

        # names can contains space ("EM Bary")
        name, *str_elements = re.split(r'\s{2,}', first_line)
        str_changes = second_line.split()

        # parse values
        elements = [float(element) for element in str_elements]
        changes = [float(change) for change in str_changes]

        # convert denominator of rates of change from century to second
        changes = [change / century for change in changes]

        # unpack
        (
            semi_major_axis, eccentricity, inclination, mean_longitude,
            longitude_of_periapsis, longitude_of_ascending_node,
        ) = elements
        (
            r_semi_major_axis, r_eccentricity, r_inclination, r_mean_longitude,
            r_longitude_of_periapsis, r_longitude_of_ascending_node,
        ) = changes

        # normalize to SI units
        semi_major_axis *= astronomical_unit
        r_semi_major_axis *= astronomical_unit

        inclination = radians(inclination)
        r_inclination = radians(r_inclination)

        mean_longitude = radians(mean_longitude)
        r_mean_longitude = radians(r_mean_longitude)

        longitude_of_periapsis = radians(longitude_of_periapsis)
        r_longitude_of_periapsis = radians(r_longitude_of_periapsis)

        longitude_of_ascending_node = radians(longitude_of_ascending_node)
        r_longitude_of_ascending_node = radians(r_longitude_of_ascending_node)

        # export
        yield PElemDataRow(
            name=name,
            semi_major_axis=(semi_major_axis, r_semi_major_axis),
            eccentricity=(eccentricity, r_eccentricity),
            inclination=(inclination, r_inclination),
            mean_longitude=(mean_longitude, mean_longitude),
            longitude_of_periapsis=(longitude_of_periapsis, r_longitude_of_periapsis),
            longitude_of_ascending_node=(longitude_of_ascending_node, r_longitude_of_ascending_node),
        )


def parse_p_elem_t1() -> Iterator[PElemDataRow]:
    with open('data/p_elem_t1.txt') as f:
        lines = f.readlines()
    yield from parse_p_elem(lines[16:])


def parse_p_elem_t2() -> Iterator[PElemDataRow]:
    with open('data/p_elem_t2.txt') as f:
        lines = f.readlines()
    yield from parse_p_elem(lines[17:35])


def parse_sat_phys_par() -> Iterator[SatPhysParDataRow]:
    with open('data/sat_phys_par.html') as f:
        html = f.read()

    # iterate over systems
    matches = re.findall(r'(?s)<P><font size=\+1><b>([^<]*)</b></font>(.*?)</TABLE>', html)
    for title, table in matches:
        primary = {
            "Earth's Moon": 'Earth',
            'Martian System': 'Mars',
            'Jovian System': 'Jupiter',
            'Saturnian System': 'Saturn',
            'Uranian Satellites': 'Uranus',
            'Neptunian Satellites': 'Neptune',
            "Pluto's Satellites": 'Pluto',
        }[title]

        # iterate over moons
        for row in re.findall(r'(?s)<TR ALIGN=right>(.*?</TD>)(?!\s*<TD)', table):
            data_cells = re.findall(r'<TD[^>]*>(.*?)</TD>', row)
            name, *values = data_cells
            name = name.strip()

            # S/2003 J1 -> S/2003 J 1
            match = re.match(r'(S/\d{4} [A-Z])(\d*)$', name)
            if match is not None:
                name = match.group(1) + ' ' + match.group(2)

            # drop precision information
            values = [
                value.split('&#177;')[0]  # HTML entity for "±"
                for value in values
            ]

            # unpack
            (
                gravitational_parameter, _,
                mean_radius, _,
                mean_density,
                magnitude, _,
                geometric_albedo, _,
            ) = values

            # normalize to SI units and export
            yield SatPhysParDataRow(
                name=name,
                primary=primary,
                gravitational_parameter=float(gravitational_parameter) * 1e9,
                mean_radius=float(mean_radius) * 1e3,
                mean_density=None if mean_density == '?' else float(mean_density) * 1e3,
                magnitude=magnitude,
                geometric_albedo=None if geometric_albedo == '?' else float(geometric_albedo),
            )


def parse_sat_elem() -> Iterator[SatElemDataRow]:
    with open('data/sat_elem.html') as f:
        html = f.read()

    # iterate over systems
    for primary, system in re.findall(r'(?s)<b>Satellites of ([^<]*)<(.*?)</table>', html):

        # iterate over orbit sets
        pattern = r'(?s)<H3>(.*?)</H3>(?:\nEpoch ([^<]*?)<BR>)?(.*?)</TABLE>'
        for reference_plane_info, epoch, orbit_set in re.findall(pattern, system):
            if 'ecliptic' in reference_plane_info.lower():
                reference_plane = ReferencePlane.ECLIPTIC
            elif 'Laplace' in reference_plane_info:
                reference_plane = ReferencePlane.LAPLACE
            elif 'equatorial' in reference_plane_info:
                # NOTE: the elements of Pluto's moons are barycentric except
                # for Charon, for which they are plateocentric
                reference_plane = ReferencePlane.EQUATORIAL
            else:
                assert False

            # missing epoch
            if not epoch and primary == 'Pluto':
                epoch = '2013 Jan. 1.00 TT'  # from PLU042

            # parse epoch
            # NOTE: TT = Terretrial Time
            #       TDT = Terrestrial Dynamical Time
            # See <https://en.wikipedia.org/wiki/Terrestrial_Time>
            pattern = r'(\d{4}) *([A-Z][a-z]{2})\. *(\d+\.\d+) TD?T$'
            m = re.match(pattern, epoch)
            assert m is not None
            year, month, day = m.groups()
            months_abbreviations = [
                'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep',
                'Oct', 'Nov', 'Dec',
            ]
            month = 1 + months_abbreviations.index(month)
            epoch = datetime(int(year), month, 1) + timedelta(days=float(day) - 1)

            # iterate over moons

            for orbit in re.findall(r'(?s)<TR ALIGN=right.*?</TR>', orbit_set):
                name, *elements, ref = re.findall(r'<TD[^>]*>(.*?)</TD>', orbit)

                # unpack
                (
                    semi_major_axis,
                    eccentricity,
                    argument_of_periapsis,
                    mean_anomaly_at_epoch,
                    inclination,
                    longitude_of_ascending_node,
                    longitude_rate,
                    period,
                    argument_of_periapsis_precession_period,
                    longitude_of_ascending_node_precession_period,
                    *laplace_elements,
                ) = [float(element) for element in elements]

                # special case for Laplace reference plane
                north_pole_right_ascension: Optional[float]
                north_pole_declination: Optional[float]
                tilt: Optional[float]
                if reference_plane == 'laplace':
                    laplace_elements = [radians(element) for element in laplace_elements]
                    north_pole_right_ascension, north_pole_declination, tilt = laplace_elements
                else:
                    north_pole_right_ascension = None
                    north_pole_declination = None
                    tilt = None

                # normalize to SI units and export
                yield SatElemDataRow(
                    name=name,
                    primary=primary,
                    reference_plane=reference_plane,
                    epoch=epoch,
                    semi_major_axis=semi_major_axis * 1e3,
                    eccentricity=eccentricity,
                    argument_of_periapsis=radians(argument_of_periapsis),
                    mean_anomaly_at_epoch=radians(mean_anomaly_at_epoch),
                    inclination=radians(inclination),
                    longitude_of_ascending_node=radians(longitude_of_ascending_node),
                    longitude_rate=radians(longitude_rate) / 86400,
                    period=period * 86400,
                    argument_of_periapsis_precession_period=(
                        argument_of_periapsis_precession_period * 365.25 * 86400
                    ),
                    longitude_of_ascending_node_precession_period=(
                        longitude_of_ascending_node_precession_period * 365.25 * 86400
                    ),
                    north_pole_right_ascension=north_pole_right_ascension,
                    north_pole_declination=north_pole_declination,
                    tilt=tilt,
                )


def parse_sbdb(name: str) -> SBDBDataRow:
    with open(f'data/sbdb/{name}.html') as f:
        html = f.read()

    parameters: Dict = {
        'name': name,
        'primary': 'Sun',
    }

    # pattern = r'<tr.*>(.*)</a></font></td> <td.*?><font.*?>(.*?)</font></td>'
    # orbital elements table
    match = re.search(r'(?s)Orbital Elements at Epoch (\d+\.\d+) .*</table>', html)
    assert match is not None
    orbital_elements = match.group(0)
    julian_epoch = float(match.group(1))

    # TDB = Barycentric Dynamical Time (from French Temps Dynamique Barycentrique)
    # essentially the same as TT and TDT
    julian_J2000 = 2451545.0
    J2000 = datetime(2000, 1, 1, 12)
    parameters['epoch'] = J2000 + timedelta(days=julian_epoch - julian_J2000)

    # iterate over orbital elements rows
    pattern = r'">([^>]*?)</a>.*?">(\d*\.\d+)'
    for parameter, value in re.findall(pattern, orbital_elements):
        if parameter == 'e':
            parameters['eccentricity'] = float(value)
        elif parameter == 'a':
            parameters['semi_major_axis'] = float(value) * astronomical_unit
        elif parameter == 'q':
            parameters['periapsis'] = float(value) * astronomical_unit
        elif parameter == 'i':
            parameters['inclination'] = radians(float(value))
        elif parameter == 'node':
            parameters['longitude_of_ascending_node'] = radians(float(value))
        elif parameter == 'peri':
            parameters['argument_of_periapsis'] = radians(float(value))
        elif parameter == 'M':
            parameters['mean_anomaly_at_epoch'] = radians(float(value))
        elif parameter == 't':  # t_p, time to periapsis
            pass
        elif parameter == 'period':
            parameters['orbital_period'] = float(value)
        elif parameter == 'n':
            parameters['mean_motion'] = radians(float(value)) / 86400
        elif parameter == 'Q':
            parameters['apoapsis'] = float(value) * astronomical_unit
        else:
            print(f'Parsing SBDB orbital elements for {name}, dit not expect "{parameter}" ({value})')

    # physical parameters table
    match = re.search(r'(?s)<b>Physical Parameter Table.*?</table>', html)
    assert match is not None
    physical_parameters = match.group(0)

    # iterate over physical parameters rows
    # alright, *maybe* I should be using bs4 for this
    pattern = r'<tr>\n  <td.*>([^<]*)</font></a></td>\n.*\n  <td.*>([^<]*)</font></td>'
    for parameter, value in re.findall(pattern, physical_parameters):
        if parameter == 'absolute magnitude':
            parameters['absolute_magnitude'] = float(value)
        elif parameter == 'magnitude slope':
            parameters['magnitude_slope'] = float(value)
        elif parameter == 'diameter':
            parameters['radius'] = float(value) * 1e3 / 2.
        elif parameter == 'extent':
            pass
        elif parameter == 'GM':
            parameters['gravitational_parameter'] = float(value) * 1e9
        elif parameter == 'bulk density':
            parameters['bulk_density'] = float(value) * 1e3
        elif parameter == 'rotation period':
            parameters['rotation_period'] = float(value) * 3600.
            if name == 'Pluto':
                print('X', parameters['rotation_period'])
        elif parameter == 'pole direction':
            ra, dec = value.split('/')
            parameters['north_pole_right_ascension'] = float(ra)
            parameters['north_pole_declination'] = float(dec)
        elif parameter == 'geometric albedo':
            parameters['geometric_albedo'] = float(value)
        elif parameter == 'B-V':
            pass
        elif parameter == 'U-B':
            pass
        elif parameter == 'Tholen spectral type':
            pass
        elif parameter == 'SMASSII spectral type':
            pass
        else:
            print(f'Parsing SBDB physical parameters for {name}, dit not expect "{parameter}" ({value})')

    return SBDBDataRow(**parameters)


def main() -> None:
    row: Any = None
    print(planet_phys_par_G())

    for row in parse_planet_phys_par():
        print(row)

    for row in parse_factsheets():
        print(row)

    for row in parse_p_elem_t1():
        print(row)

    for row in parse_p_elem_t2():
        print(row)

    for row in parse_sat_phys_par():
        print(row)

    for row in parse_sat_elem():
        print(row)

    dwarf_planets = ('Ceres', 'Pluto', 'Sedna', 'Haumea', 'Makemake', 'Eris')
    for dwarf_planet in dwarf_planets:
        print(parse_sbdb(dwarf_planet))


if __name__ == '__main__':
    main()
