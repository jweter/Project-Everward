from __future__ import annotations

import sys
from pathlib import Path
import unittest


THIS_DIR = Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))

from runner import HeadlessSimulation, HeadlessSnapshot, TICKS_PER_JULIAN_YEAR


def _valid_snapshot(**overrides) -> HeadlessSnapshot:
    """A structurally valid snapshot with 1 simulated year still remaining."""

    fields = {
        "seed": 1,
        "horizon_tick": 10 * TICKS_PER_JULIAN_YEAR,
        "current_tick": 9 * TICKS_PER_JULIAN_YEAR,
        "state": {
            "maintenance_cycles": 9,
            "survey_cycles": 0,
            "archive_cycles": 0,
            "deterministic_accumulator": 0,
        },
        "next_due": {
            "maintenance": 10 * TICKS_PER_JULIAN_YEAR,
            "survey": 10 * TICKS_PER_JULIAN_YEAR,
            "archive": 100 * TICKS_PER_JULIAN_YEAR,
        },
    }
    fields.update(overrides)
    return HeadlessSnapshot(**fields)


class HeadlessSimulationTests(unittest.TestCase):
    def test_same_seed_and_horizon_replay_exactly(self) -> None:
        first = HeadlessSimulation(seed=847291, years=10_000).run()
        second = HeadlessSimulation(seed=847291, years=10_000).run()
        self.assertEqual(first.canonical_json(), second.canonical_json())
        self.assertEqual(first.fingerprint, second.fingerprint)

    def test_different_seed_changes_deterministic_state(self) -> None:
        first = HeadlessSimulation(seed=847291, years=1_000).run()
        second = HeadlessSimulation(seed=847292, years=1_000).run()
        self.assertNotEqual(
            first.deterministic_accumulator,
            second.deterministic_accumulator,
        )
        self.assertNotEqual(first.fingerprint, second.fingerprint)

    def test_ten_thousand_year_horizon_processes_expected_sparse_workload(self) -> None:
        summary = HeadlessSimulation(seed=1, years=10_000).run()
        self.assertEqual(summary.final_tick, 10_000 * TICKS_PER_JULIAN_YEAR)
        self.assertEqual(summary.maintenance_cycles, 10_000)
        self.assertEqual(summary.survey_cycles, 1_000)
        self.assertEqual(summary.archive_cycles, 100)
        self.assertEqual(summary.processed_events, 11_100)
        self.assertEqual(summary.pending_events, 0)

    def test_checkpoint_round_trip_matches_uninterrupted_run(self) -> None:
        uninterrupted = HeadlessSimulation(seed=424242, years=10_000).run()

        partial = HeadlessSimulation(seed=424242, years=10_000)
        partial.advance_to_year(4_321)
        serialized = partial.snapshot().canonical_json()
        restored_snapshot = HeadlessSnapshot.from_json(serialized)
        resumed = HeadlessSimulation.from_snapshot(restored_snapshot).run()

        self.assertEqual(uninterrupted.canonical_json(), resumed.canonical_json())
        self.assertEqual(uninterrupted.fingerprint, resumed.fingerprint)

    def test_snapshot_preserves_next_event_schedule(self) -> None:
        simulation = HeadlessSimulation(seed=7, years=500)
        simulation.advance_to_year(155)
        snapshot = simulation.snapshot()
        self.assertEqual(snapshot.current_tick, 155 * TICKS_PER_JULIAN_YEAR)
        self.assertEqual(snapshot.next_due["maintenance"], 156 * TICKS_PER_JULIAN_YEAR)
        self.assertEqual(snapshot.next_due["survey"], 160 * TICKS_PER_JULIAN_YEAR)
        self.assertEqual(snapshot.next_due["archive"], 200 * TICKS_PER_JULIAN_YEAR)

    def test_invalid_horizon_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            HeadlessSimulation(seed=1, years=0)


class HeadlessSimulationGuardClauseTests(unittest.TestCase):
    """Cover the constructor/restore/advance guard clauses per rule.

    `test_invalid_horizon_is_rejected` above only ever supplies `years=0`, and
    no existing test ever calls `HeadlessSimulation.from_snapshot` or
    `advance_to_year` with an invalid argument -- both guard clauses had zero
    coverage. These tests exercise every declared branch of each guard
    independently, matching the per-field/per-key coverage audit already
    applied to `prototypes/coordinate-scale` and `prototypes/simulation-clock`.
    """

    def test_constructor_rejects_non_positive_years_at_both_boundaries(self) -> None:
        for years in (0, -1, -10_000):
            with self.subTest(years=years):
                with self.assertRaisesRegex(ValueError, "years must be positive"):
                    HeadlessSimulation(seed=1, years=years)

    def test_from_snapshot_rejects_a_snapshot_with_no_time_remaining(self) -> None:
        exhausted = _valid_snapshot(
            horizon_tick=5 * TICKS_PER_JULIAN_YEAR,
            current_tick=5 * TICKS_PER_JULIAN_YEAR,
        )
        overshot = _valid_snapshot(
            horizon_tick=5 * TICKS_PER_JULIAN_YEAR,
            current_tick=6 * TICKS_PER_JULIAN_YEAR,
        )
        for snapshot in (exhausted, overshot):
            with self.subTest(current_tick=snapshot.current_tick):
                with self.assertRaisesRegex(
                    ValueError, "snapshot must have simulation time remaining"
                ):
                    HeadlessSimulation.from_snapshot(snapshot)

    def test_from_snapshot_rejects_a_horizon_not_aligned_to_whole_years(self) -> None:
        misaligned = _valid_snapshot(horizon_tick=10 * TICKS_PER_JULIAN_YEAR + 1)
        with self.assertRaisesRegex(
            ValueError, "snapshot horizon must align to whole Julian years"
        ):
            HeadlessSimulation.from_snapshot(misaligned)

    def test_from_snapshot_accepts_a_valid_snapshot(self) -> None:
        simulation = HeadlessSimulation.from_snapshot(_valid_snapshot())
        self.assertEqual(simulation.clock.tick, 9 * TICKS_PER_JULIAN_YEAR)
        self.assertEqual(simulation.horizon_tick, 10 * TICKS_PER_JULIAN_YEAR)

    def test_advance_to_year_rejects_a_year_outside_the_configured_horizon(self) -> None:
        simulation = HeadlessSimulation(seed=1, years=100)
        for year in (-1, -1000, 101, 1_000):
            with self.subTest(year=year):
                with self.assertRaisesRegex(
                    ValueError, "year must be inside the configured horizon"
                ):
                    simulation.advance_to_year(year)

    def test_advance_to_year_accepts_both_inclusive_boundaries(self) -> None:
        simulation = HeadlessSimulation(seed=1, years=100)
        self.assertEqual(simulation.advance_to_year(0), 0)
        simulation.advance_to_year(100)
        self.assertEqual(simulation.clock.tick, 100 * TICKS_PER_JULIAN_YEAR)


if __name__ == "__main__":
    unittest.main()
