"""Golden-seed regression bank for the procedural star-system generator.

docs/TESTING_STRATEGY.md requires a compact bank of known seeds/coordinates
covering ordinary, sparse, resource-rich, extreme, and numerical-edge cases so
that generation tests do not all depend on a single seed. The property-based
tests in test_generator.py prove invariants across many seeds but never pin
an exact expected output, so an unintended change to generator.py could
silently change canonical output for an existing generator_version without
any test failing. This module closes that gap: it replays each fixture case
in golden_seeds.json and asserts the exact canonical JSON and fingerprint the
generator produced when the fixture was authored.

A deliberate generation-algorithm change must bump GENERATOR_VERSION and
regenerate golden_seeds.json from the new algorithm; it must never be hand
edited to make a failing case pass. That keeps the versioned-identity
contract in generator.py's module docstring honest.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

from generator import GENERATOR_VERSION, STAR_TABLE, generate_system

FIXTURE_PATH = Path(__file__).parent / "golden_seeds.json"
STAR_TABLE_CLASSES = {spectral_class for spectral_class, _weight in STAR_TABLE}


class GoldenSeedTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        with FIXTURE_PATH.open("r", encoding="utf-8") as handle:
            cls.fixture = json.load(handle)

    def test_fixture_is_pinned_to_the_current_generator_version(self) -> None:
        self.assertEqual(
            self.fixture["generator_version"],
            GENERATOR_VERSION,
            "golden_seeds.json is pinned to a different generator_version than "
            "generator.py; a deliberate algorithm change must regenerate this "
            "fixture from the new GENERATOR_VERSION rather than leaving it stale.",
        )

    def test_fixture_covers_a_diverse_set_of_cases(self) -> None:
        labels = {case["label"] for case in self.fixture["cases"]}
        self.assertGreaterEqual(
            len(labels),
            5,
            "golden-seed bank should not collapse onto a single case; see "
            "docs/TESTING_STRATEGY.md's Golden-seed tests section.",
        )
        self.assertEqual(len(labels), len(self.fixture["cases"]), "case labels must be unique")

    def test_each_pinned_seed_reproduces_its_recorded_canonical_output(self) -> None:
        for case in self.fixture["cases"]:
            with self.subTest(case=case["label"]):
                system = generate_system(
                    case["universe_seed"],
                    tuple(case["coordinate"]),
                    generator_version=case["generator_version"],
                )
                self.assertEqual(
                    json.loads(system.canonical_json()),
                    case["expected_system"],
                    f"canonical output for golden case {case['label']!r} drifted from the "
                    "pinned fixture; regenerate golden_seeds.json only after a deliberate, "
                    "version-bumped generator change",
                )
                self.assertEqual(system.fingerprint(), case["expected_fingerprint"])

    def test_sparse_case_has_no_planets(self) -> None:
        case = self._case("sparse_no_planets")
        self.assertEqual(case["expected_system"]["planets"], [])

    def test_resource_rich_case_has_many_bodies(self) -> None:
        case = self._case("resource_rich_multi_body")
        system = case["expected_system"]
        total_moons = sum(len(planet["moons"]) for planet in system["planets"])
        self.assertGreaterEqual(len(system["planets"]), 9)
        self.assertGreaterEqual(total_moons, 30)
        self.assertGreaterEqual(len(system["belts"]), 2)

    def test_rare_spectral_classes_are_represented(self) -> None:
        classes = {case["label"]: case["expected_system"]["star"]["spectral_class"] for case in self.fixture["cases"]}
        self.assertEqual(classes["rare_o_class_star"], "O")
        self.assertEqual(classes["rare_b_class_star"], "B")
        self.assertEqual(classes["rare_a_class_star"], "A")

    def test_every_star_table_spectral_class_is_pinned_somewhere_in_the_bank(self) -> None:
        # STAR_TABLE (generator.py) declares seven weighted spectral classes.
        # A per-call range check can only validate a value against the same
        # STAR_RANGES entry the generator itself used to produce it, so it
        # cannot catch an unintended change that reshuffles which class a
        # given roll selects. Pinning at least one golden case per class
        # closes that blind spot for the whole table, not just the rarest
        # two entries.
        pinned_classes = {case["expected_system"]["star"]["spectral_class"] for case in self.fixture["cases"]}
        self.assertEqual(pinned_classes, set(STAR_TABLE_CLASSES))

    def test_interstellar_scale_case_uses_large_mixed_sign_coordinate(self) -> None:
        case = self._case("interstellar_scale_negative_coordinate")
        coordinate = case["coordinate"]
        self.assertTrue(any(abs(component) >= 100_000 for component in coordinate))
        self.assertTrue(any(component < 0 for component in coordinate))

    def _case(self, label: str) -> dict:
        for case in self.fixture["cases"]:
            if case["label"] == label:
                return case
        raise AssertionError(f"golden case {label!r} not found in fixture")


if __name__ == "__main__":
    unittest.main()
