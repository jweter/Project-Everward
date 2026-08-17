from __future__ import annotations

import sys
from pathlib import Path
import unittest


THIS_DIR = Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))

from runner import HeadlessSimulation, HeadlessSnapshot, TICKS_PER_JULIAN_YEAR


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


if __name__ == "__main__":
    unittest.main()
