"""Deterministic procedural astronomy reference prototype for Everward.

Generation contract:
    universe_seed + spatial_coordinate + generation_algorithm_version -> StarSystem

The prototype uses only the Python standard library and stores most generated
quantities as scaled integers so canonical output is stable across platforms.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
import json
import math
from typing import Iterable

GENERATOR_VERSION = 1


class DeterministicStream:
    """SHA-256 counter stream for stable integer sampling."""

    def __init__(self, key: str) -> None:
        self._key = key.encode("utf-8")
        self._counter = 0

    def _word(self) -> int:
        payload = self._key + b"|" + str(self._counter).encode("ascii")
        self._counter += 1
        return int.from_bytes(hashlib.sha256(payload).digest()[:8], "big")

    def below(self, upper_exclusive: int) -> int:
        if upper_exclusive <= 0:
            raise ValueError("upper_exclusive must be positive")
        return self._word() % upper_exclusive

    def between(self, low: int, high: int) -> int:
        if high < low:
            raise ValueError("high must be >= low")
        return low + self.below(high - low + 1)

    def weighted(self, choices: Iterable[tuple[str, int]]) -> str:
        values = list(choices)
        total = sum(weight for _, weight in values)
        if total <= 0:
            raise ValueError("weighted choices require positive total weight")
        pick = self.below(total)
        cumulative = 0
        for value, weight in values:
            cumulative += weight
            if pick < cumulative:
                return value
        raise AssertionError("unreachable weighted selection")


@dataclass(frozen=True, slots=True)
class ResourceProfile:
    iron_bp: int
    nickel_bp: int
    silicates_bp: int
    water_bp: int
    carbon_bp: int
    volatiles_bp: int
    radioactives_bp: int
    rare_metals_bp: int

    def total_basis_points(self) -> int:
        return sum(asdict(self).values())


@dataclass(frozen=True, slots=True)
class Moon:
    entity_id: str
    ordinal: int
    radius_km: int
    orbit_radius_km: int
    resources: ResourceProfile


@dataclass(frozen=True, slots=True)
class Planet:
    entity_id: str
    ordinal: int
    planet_type: str
    semi_major_axis_milli_au: int
    mass_milli_earth: int
    radius_km: int
    equilibrium_temp_k: int
    moons: tuple[Moon, ...]
    resources: ResourceProfile


@dataclass(frozen=True, slots=True)
class AsteroidBelt:
    entity_id: str
    ordinal: int
    inner_milli_au: int
    outer_milli_au: int
    estimated_mass_millionths_earth: int
    resources: ResourceProfile


@dataclass(frozen=True, slots=True)
class Star:
    entity_id: str
    spectral_class: str
    mass_milli_solar: int
    radius_milli_solar: int
    temperature_k: int
    luminosity_milli_solar: int


@dataclass(frozen=True, slots=True)
class StarSystem:
    system_id: str
    universe_seed: int
    coordinate: tuple[int, int, int]
    generator_version: int
    star: Star
    planets: tuple[Planet, ...]
    belts: tuple[AsteroidBelt, ...]

    def canonical_json(self) -> str:
        return json.dumps(asdict(self), sort_keys=True, separators=(",", ":"))

    def fingerprint(self) -> str:
        return hashlib.sha256(self.canonical_json().encode("utf-8")).hexdigest()


STAR_TABLE: tuple[tuple[str, int], ...] = (
    ("M", 7200),
    ("K", 1200),
    ("G", 750),
    ("F", 450),
    ("A", 250),
    ("B", 120),
    ("O", 30),
)

STAR_RANGES = {
    "M": ((80, 600), (100, 700), (2400, 3900), (1, 180)),
    "K": ((600, 900), (650, 950), (3900, 5200), (180, 650)),
    "G": ((900, 1100), (900, 1150), (5200, 6000), (650, 1500)),
    "F": ((1100, 1500), (1150, 1600), (6000, 7500), (1500, 5000)),
    "A": ((1500, 2400), (1400, 2500), (7500, 10_000), (5000, 25_000)),
    "B": ((2400, 16_000), (2000, 7000), (10_000, 30_000), (25_000, 100_000_000)),
    "O": ((16_000, 60_000), (6000, 20_000), (30_000, 55_000), (100_000_000, 1_000_000_000)),
}


def stable_id(root_key: str, path: str) -> str:
    digest = hashlib.sha256(f"{root_key}|{path}".encode("utf-8")).hexdigest()
    return f"ev-{digest[:16]}"


def _normalized_resources(stream: DeterministicStream, bias: str) -> ResourceProfile:
    weights = {
        "iron": stream.between(300, 2200),
        "nickel": stream.between(80, 900),
        "silicates": stream.between(1200, 5000),
        "water": stream.between(0, 3500),
        "carbon": stream.between(50, 1400),
        "volatiles": stream.between(0, 1600),
        "radioactives": stream.between(5, 180),
        "rare_metals": stream.between(5, 220),
    }
    if bias == "metallic":
        weights["iron"] *= 2
        weights["nickel"] *= 2
        weights["rare_metals"] *= 2
    elif bias == "icy":
        weights["water"] *= 3
        weights["volatiles"] *= 2
    elif bias == "carbonaceous":
        weights["carbon"] *= 3
        weights["water"] *= 2

    total = sum(weights.values())
    normalized = {name: value * 10_000 // total for name, value in weights.items()}
    normalized["silicates"] += 10_000 - sum(normalized.values())
    return ResourceProfile(
        iron_bp=normalized["iron"],
        nickel_bp=normalized["nickel"],
        silicates_bp=normalized["silicates"],
        water_bp=normalized["water"],
        carbon_bp=normalized["carbon"],
        volatiles_bp=normalized["volatiles"],
        radioactives_bp=normalized["radioactives"],
        rare_metals_bp=normalized["rare_metals"],
    )


def _planet_type(stream: DeterministicStream, axis_milli_au: int, luminosity_milli: int) -> str:
    scaled_heat = luminosity_milli * 1000 // max(axis_milli_au * axis_milli_au, 1)
    roll = stream.below(100)
    if axis_milli_au > 3500:
        return "ice" if roll < 40 else "ice_giant" if roll < 75 else "dwarf"
    if axis_milli_au > 1200:
        return "gas_giant" if roll < 38 else "ice_giant" if roll < 58 else "rocky"
    if scaled_heat < 900 and roll < 18:
        return "oceanic"
    return "rocky" if roll < 82 else "dwarf"


def _planet_properties(stream: DeterministicStream, planet_type: str) -> tuple[int, int]:
    if planet_type == "gas_giant":
        return stream.between(30_000, 420_000), stream.between(35_000, 90_000)
    if planet_type == "ice_giant":
        return stream.between(8_000, 45_000), stream.between(18_000, 35_000)
    if planet_type == "dwarf":
        return stream.between(1, 120), stream.between(300, 3_000)
    if planet_type == "oceanic":
        return stream.between(500, 6_000), stream.between(4_500, 14_000)
    if planet_type == "ice":
        return stream.between(80, 3_000), stream.between(2_000, 12_000)
    return stream.between(80, 8_000), stream.between(2_000, 15_000)


def _temperature_k(luminosity_milli: int, axis_milli_au: int) -> int:
    # Deliberately integer-only: fourth root of scaled luminosity divided by
    # square root of distance. This is a simplified equilibrium-temperature
    # proxy, not a final astrophysics model.
    luminosity_factor = max(1, math.isqrt(math.isqrt(luminosity_milli * 1000)))
    distance_factor = max(1, math.isqrt(axis_milli_au))
    return max(3, 278 * luminosity_factor // distance_factor)


def generate_system(
    universe_seed: int,
    coordinate: tuple[int, int, int],
    *,
    generator_version: int = GENERATOR_VERSION,
) -> StarSystem:
    if len(coordinate) != 3:
        raise ValueError("coordinate must contain exactly three integers")
    if generator_version <= 0:
        raise ValueError("generator_version must be positive")

    root_key = f"everward|{universe_seed}|{coordinate[0]},{coordinate[1]},{coordinate[2]}|v{generator_version}"
    stream = DeterministicStream(root_key)
    system_id = stable_id(root_key, "system")

    spectral_class = stream.weighted(STAR_TABLE)
    mass_range, radius_range, temp_range, lum_range = STAR_RANGES[spectral_class]
    star = Star(
        entity_id=stable_id(root_key, "star"),
        spectral_class=spectral_class,
        mass_milli_solar=stream.between(*mass_range),
        radius_milli_solar=stream.between(*radius_range),
        temperature_k=stream.between(*temp_range),
        luminosity_milli_solar=stream.between(*lum_range),
    )

    planet_count = stream.between(0, 10)
    planets: list[Planet] = []
    axis = stream.between(120, 450)
    for ordinal in range(1, planet_count + 1):
        if ordinal > 1:
            axis += stream.between(max(120, axis // 3), max(300, axis))
        ptype = _planet_type(stream, axis, star.luminosity_milli_solar)
        mass, radius = _planet_properties(stream, ptype)
        if ptype in {"gas_giant", "ice_giant"}:
            moon_cap = 12
        elif mass > 500:
            moon_cap = 4
        elif mass > 100:
            moon_cap = 2
        else:
            moon_cap = 0
        moon_count = stream.between(0, moon_cap) if moon_cap else 0
        moons: list[Moon] = []
        moon_orbit = radius + stream.between(2_000, 15_000)
        for moon_ordinal in range(1, moon_count + 1):
            if moon_ordinal > 1:
                moon_orbit += stream.between(5_000, 80_000)
            bias = "icy" if axis > 1800 else stream.weighted((("metallic", 1), ("carbonaceous", 2)))
            moons.append(
                Moon(
                    entity_id=stable_id(root_key, f"planet/{ordinal}/moon/{moon_ordinal}"),
                    ordinal=moon_ordinal,
                    radius_km=stream.between(150, min(4_000, max(200, radius // 3))),
                    orbit_radius_km=moon_orbit,
                    resources=_normalized_resources(stream, bias),
                )
            )
        resource_bias = "icy" if ptype in {"ice", "ice_giant"} else "metallic" if ptype == "rocky" else "carbonaceous"
        planets.append(
            Planet(
                entity_id=stable_id(root_key, f"planet/{ordinal}"),
                ordinal=ordinal,
                planet_type=ptype,
                semi_major_axis_milli_au=axis,
                mass_milli_earth=mass,
                radius_km=radius,
                equilibrium_temp_k=_temperature_k(star.luminosity_milli_solar, axis),
                moons=tuple(moons),
                resources=_normalized_resources(stream, resource_bias),
            )
        )

    belt_count = stream.between(0, 2)
    belts: list[AsteroidBelt] = []
    for ordinal in range(1, belt_count + 1):
        center = stream.between(700, max(1200, axis + 700))
        width = stream.between(100, 900)
        bias = stream.weighted((("metallic", 30), ("carbonaceous", 45), ("icy", 25)))
        belts.append(
            AsteroidBelt(
                entity_id=stable_id(root_key, f"belt/{ordinal}"),
                ordinal=ordinal,
                inner_milli_au=max(80, center - width // 2),
                outer_milli_au=center + width // 2,
                estimated_mass_millionths_earth=stream.between(1, 50_000),
                resources=_normalized_resources(stream, bias),
            )
        )
    belts.sort(key=lambda belt: (belt.inner_milli_au, belt.entity_id))

    return StarSystem(
        system_id=system_id,
        universe_seed=universe_seed,
        coordinate=coordinate,
        generator_version=generator_version,
        star=star,
        planets=tuple(planets),
        belts=tuple(belts),
    )
