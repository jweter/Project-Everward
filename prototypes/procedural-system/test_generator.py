from __future__ import annotations

import unittest

from generator import STAR_RANGES, DeterministicStream, generate_system


class ProceduralSystemTests(unittest.TestCase):
    def test_identical_inputs_reproduce_identical_output(self) -> None:
        left = generate_system(847291, (18, -4, 71))
        right = generate_system(847291, (18, -4, 71))
        self.assertEqual(left, right)
        self.assertEqual(left.canonical_json(), right.canonical_json())
        self.assertEqual(left.fingerprint(), right.fingerprint())

    def test_changed_coordinates_create_distinct_systems(self) -> None:
        a = generate_system(847291, (18, -4, 71))
        b = generate_system(847291, (18, -4, 72))
        self.assertNotEqual(a.system_id, b.system_id)
        self.assertNotEqual(a.fingerprint(), b.fingerprint())

    def test_generator_version_participates_in_identity(self) -> None:
        a = generate_system(42, (0, 0, 0), generator_version=1)
        b = generate_system(42, (0, 0, 0), generator_version=2)
        self.assertNotEqual(a.system_id, b.system_id)
        self.assertNotEqual(a.fingerprint(), b.fingerprint())

    def test_star_properties_stay_inside_declared_class_ranges(self) -> None:
        for index in range(250):
            system = generate_system(1001, (index, index // 7, -index))
            mass_range, radius_range, temp_range, lum_range = STAR_RANGES[system.star.spectral_class]
            self.assertGreaterEqual(system.star.mass_milli_solar, mass_range[0])
            self.assertLessEqual(system.star.mass_milli_solar, mass_range[1])
            self.assertGreaterEqual(system.star.radius_milli_solar, radius_range[0])
            self.assertLessEqual(system.star.radius_milli_solar, radius_range[1])
            self.assertGreaterEqual(system.star.temperature_k, temp_range[0])
            self.assertLessEqual(system.star.temperature_k, temp_range[1])
            self.assertGreaterEqual(system.star.luminosity_milli_solar, lum_range[0])
            self.assertLessEqual(system.star.luminosity_milli_solar, lum_range[1])

    def test_planet_orbits_are_strictly_increasing(self) -> None:
        for index in range(100):
            system = generate_system(99, (index, 2, 3))
            axes = [planet.semi_major_axis_milli_au for planet in system.planets]
            self.assertEqual(axes, sorted(axes))
            self.assertEqual(len(axes), len(set(axes)))

    def test_moon_orbits_are_strictly_increasing(self) -> None:
        for index in range(100):
            system = generate_system(7654, (index, -1, 9))
            for planet in system.planets:
                orbits = [moon.orbit_radius_km for moon in planet.moons]
                self.assertEqual(orbits, sorted(orbits))
                self.assertEqual(len(orbits), len(set(orbits)))

    def test_resource_profiles_sum_to_100_percent(self) -> None:
        for index in range(100):
            system = generate_system(12345, (5, index, -8))
            for planet in system.planets:
                self.assertEqual(planet.resources.total_basis_points(), 10_000)
                for moon in planet.moons:
                    self.assertEqual(moon.resources.total_basis_points(), 10_000)
            for belt in system.belts:
                self.assertEqual(belt.resources.total_basis_points(), 10_000)

    def test_all_entity_ids_are_unique_inside_system(self) -> None:
        for index in range(100):
            system = generate_system(777, (index, index, index))
            ids = [system.system_id, system.star.entity_id]
            ids.extend(planet.entity_id for planet in system.planets)
            ids.extend(moon.entity_id for planet in system.planets for moon in planet.moons)
            ids.extend(belt.entity_id for belt in system.belts)
            self.assertEqual(len(ids), len(set(ids)))

    def test_belts_have_valid_geometry(self) -> None:
        for index in range(100):
            system = generate_system(333, (index, 0, 0))
            for belt in system.belts:
                self.assertGreaterEqual(belt.inner_milli_au, 80)
                self.assertGreater(belt.outer_milli_au, belt.inner_milli_au)
                self.assertGreater(belt.estimated_mass_millionths_earth, 0)

    def test_rejects_invalid_generator_version(self) -> None:
        with self.assertRaises(ValueError):
            generate_system(1, (0, 0, 0), generator_version=0)


class GenerateSystemGuardClauseTests(unittest.TestCase):
    """Cover `generate_system`'s two entry-point guard clauses per rule.

    `test_rejects_invalid_generator_version` above only ever supplies
    `generator_version=0`, so the negative-version half of that guard was
    untested, and no existing test ever called `generate_system` with a
    wrong-length `coordinate` -- that guard had zero coverage. These tests
    exercise every declared branch of each guard independently, matching the
    per-field/per-key and guard-clause coverage audit already applied to
    `prototypes/coordinate-scale`, `prototypes/simulation-clock`, and
    `prototypes/headless-simulation` (see `ERROR_RESOLUTION_LEDGER.md`).
    """

    def test_rejects_a_coordinate_with_the_wrong_axis_count(self) -> None:
        for coordinate in ((), (1,), (1, 2), (1, 2, 3, 4)):
            with self.subTest(coordinate=coordinate):
                with self.assertRaisesRegex(
                    ValueError, "coordinate must contain exactly three integers"
                ):
                    generate_system(1, coordinate)

    def test_accepts_a_three_axis_coordinate(self) -> None:
        # Positive control: the guard above must not reject the valid shape.
        generate_system(1, (1, 2, 3))

    def test_rejects_non_positive_generator_version_at_both_boundaries(self) -> None:
        for generator_version in (0, -1, -1000):
            with self.subTest(generator_version=generator_version):
                with self.assertRaisesRegex(
                    ValueError, "generator_version must be positive"
                ):
                    generate_system(1, (0, 0, 0), generator_version=generator_version)


class DeterministicStreamGuardClauseTests(unittest.TestCase):
    """Cover `DeterministicStream`'s three sampling-method guard clauses.

    None of these guards had any existing test coverage: every call site in
    `generator.py` and `test_generator.py` supplies valid arguments, so a
    weakened or dropped check in `below`, `between`, or `weighted` could go
    undetected.
    """

    def test_below_rejects_non_positive_upper_exclusive(self) -> None:
        for upper_exclusive in (0, -1, -50):
            with self.subTest(upper_exclusive=upper_exclusive):
                with self.assertRaisesRegex(ValueError, "upper_exclusive must be positive"):
                    DeterministicStream("key").below(upper_exclusive)

    def test_below_accepts_a_positive_upper_exclusive(self) -> None:
        value = DeterministicStream("key").below(1)
        self.assertEqual(value, 0)

    def test_between_rejects_high_below_low(self) -> None:
        for low, high in ((1, 0), (10, -10), (0, -1)):
            with self.subTest(low=low, high=high):
                with self.assertRaisesRegex(ValueError, "high must be >= low"):
                    DeterministicStream("key").between(low, high)

    def test_between_accepts_an_equal_low_and_high(self) -> None:
        value = DeterministicStream("key").between(5, 5)
        self.assertEqual(value, 5)

    def test_weighted_rejects_non_positive_total_weight(self) -> None:
        cases = ((), (("only", 0),), (("a", 1), ("b", -3)))
        for choices in cases:
            with self.subTest(choices=choices):
                with self.assertRaisesRegex(
                    ValueError, "weighted choices require positive total weight"
                ):
                    DeterministicStream("key").weighted(choices)

    def test_weighted_accepts_a_single_positive_weight(self) -> None:
        value = DeterministicStream("key").weighted((("only", 1),))
        self.assertEqual(value, "only")


if __name__ == "__main__":
    unittest.main()
