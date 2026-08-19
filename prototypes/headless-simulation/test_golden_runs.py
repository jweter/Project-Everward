"""Golden-run regression bank for the headless long-horizon simulation workload.

docs/TESTING_STRATEGY.md requires a compact bank of known seeds that exercise
ordinary, sparse/minimal, boundary, and numerical-edge cases so that
determinism tests do not all collapse onto a single scenario. test_headless.py
proves strong invariants (identical seed/horizon replays exactly, a
checkpointed run matches an uninterrupted run, exact event counts for one
specific horizon) but every one of those assertions is self-referential: it
compares HeadlessSimulation's output against another call to the same code,
never against a value pinned before the comparison existed. An unintended
change to the deterministic-accumulator formula in runner.py (for example,
a different hash-digest slice, byte order, or input format) would still
satisfy every existing assertion because both sides of each comparison move
together, and no test would fail.

This module closes that gap the same way prototypes/procedural-system/
test_golden_seeds.py closes it for procedural generation: it replays each
fixture case in golden_runs.json and asserts the exact canonical summary and
fingerprint recorded when the fixture was authored.

A deliberate, intentional change to the workload's mechanical behavior must
regenerate golden_runs.json from the new behavior; it must never be hand
edited to make a failing case pass.
"""

from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest

THIS_DIR = Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))

from runner import HeadlessSimulation

FIXTURE_PATH = THIS_DIR / "golden_runs.json"


class GoldenRunTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        with FIXTURE_PATH.open("r", encoding="utf-8") as handle:
            cls.fixture = json.load(handle)

    def test_fixture_covers_a_diverse_set_of_cases(self) -> None:
        labels = {case["label"] for case in self.fixture["cases"]}
        self.assertGreaterEqual(
            len(labels),
            5,
            "golden-run bank should not collapse onto a single case; see "
            "docs/TESTING_STRATEGY.md's Golden-seed tests section.",
        )
        self.assertEqual(len(labels), len(self.fixture["cases"]), "case labels must be unique")

    def test_each_pinned_case_reproduces_its_recorded_canonical_output(self) -> None:
        for case in self.fixture["cases"]:
            with self.subTest(case=case["label"]):
                summary = HeadlessSimulation(seed=case["seed"], years=case["years"]).run()
                self.assertEqual(
                    json.loads(summary.canonical_json()),
                    case["expected_summary"],
                    f"canonical output for golden case {case['label']!r} drifted from the "
                    "pinned fixture; regenerate golden_runs.json only after a deliberate "
                    "change to the workload's mechanical behavior",
                )
                self.assertEqual(summary.fingerprint, case["expected_fingerprint"])

    def test_minimal_horizon_case_fires_only_the_maintenance_stream(self) -> None:
        case = self._case("minimal_single_maintenance_year")
        summary = case["expected_summary"]
        self.assertEqual(summary["maintenance_cycles"], 1)
        self.assertEqual(summary["survey_cycles"], 0)
        self.assertEqual(summary["archive_cycles"], 0)

    def test_exact_boundary_case_fires_archive_on_the_final_tick(self) -> None:
        case = self._case("exact_archive_boundary")
        summary = case["expected_summary"]
        self.assertEqual(summary["archive_cycles"], 1)
        self.assertEqual(summary["pending_events"], 0)

    def test_negative_and_zero_seed_cases_are_represented(self) -> None:
        seeds = {case["label"]: case["seed"] for case in self.fixture["cases"]}
        self.assertEqual(seeds["negative_seed"], -17)
        self.assertEqual(seeds["zero_seed"], 0)

    def test_large_horizon_case_matches_the_phase_1_proof_scale(self) -> None:
        case = self._case("large_horizon_ten_thousand_years")
        self.assertEqual(case["years"], 10_000)
        self.assertEqual(case["expected_summary"]["processed_events"], 11_100)

    def _case(self, label: str) -> dict:
        for case in self.fixture["cases"]:
            if case["label"] == label:
                return case
        raise AssertionError(f"golden case {label!r} not found in fixture")


if __name__ == "__main__":
    unittest.main()
