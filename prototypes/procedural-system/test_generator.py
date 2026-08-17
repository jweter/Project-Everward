from __future__ import annotations

import unittest

from generator import STAR_RANGES, generate_system


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


if __name__ == "__main__":
    unittest.main()
