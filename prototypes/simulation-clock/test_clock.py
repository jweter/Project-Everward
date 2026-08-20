from __future__ import annotations

from fractions import Fraction
import unittest

from clock import ScheduledEvent, SimulationClock, days, julian_years, seconds


class SimulationClockTests(unittest.TestCase):
    def test_simultaneous_events_preserve_insertion_order(self) -> None:
        clock = SimulationClock()
        clock.schedule_at(100, "first", {"value": 1})
        clock.schedule_at(100, "second", {"value": 2})
        clock.schedule_at(100, "third", {"value": 3})

        processed = clock.advance_to(100)

        self.assertEqual(processed, 3)
        self.assertEqual([item.kind for item in clock.history], ["first", "second", "third"])
        self.assertEqual(clock.tick, 100)

    def test_sparse_timeline_jumps_to_long_future_event(self) -> None:
        clock = SimulationClock()
        target = julian_years(10_000)
        clock.schedule_at(target, "deep_future")

        processed = clock.run_until_idle()

        self.assertEqual(processed, 1)
        self.assertEqual(clock.tick, target)
        self.assertEqual(clock.history[0].kind, "deep_future")

    def test_handler_can_schedule_followup_event(self) -> None:
        clock = SimulationClock()

        def first_handler(sim: SimulationClock, _event) -> None:
            sim.schedule_after(seconds(5), "followup", {"origin": "first"})

        clock.register_handler("first", first_handler)
        clock.schedule_after(seconds(2), "first")

        processed = clock.run_until_idle()

        self.assertEqual(processed, 2)
        self.assertEqual([item.kind for item in clock.history], ["first", "followup"])
        self.assertEqual(clock.tick, seconds(7))

    def test_pause_prevents_wall_time_advance(self) -> None:
        clock = SimulationClock()
        clock.set_paused(True)

        clock.advance_wall_ticks(seconds(10))

        self.assertEqual(clock.tick, 0)

    def test_rational_time_scale_is_exact_across_step_chunking(self) -> None:
        one_step = SimulationClock()
        many_steps = SimulationClock()
        one_step.set_time_scale(7, 3)
        many_steps.set_time_scale(7, 3)

        one_step.advance_wall_ticks(10_000)
        for _ in range(10_000):
            many_steps.advance_wall_ticks(1)

        self.assertEqual(one_step.tick, many_steps.tick)
        self.assertEqual(one_step.time_scale, Fraction(7, 3))

    def test_event_results_are_independent_of_wall_step_chunking(self) -> None:
        def make_clock() -> SimulationClock:
            clock = SimulationClock()
            clock.set_time_scale(100, 1)
            clock.schedule_at(seconds(1), "scan_complete")
            clock.schedule_at(seconds(3), "mining_complete")
            clock.schedule_at(seconds(10), "message_arrival")
            return clock

        coarse = make_clock()
        fine = make_clock()

        coarse.advance_wall_ticks(seconds(1) // 10)
        for _ in range(100):
            fine.advance_wall_ticks(seconds(1) // 1_000)

        self.assertEqual(coarse.tick, fine.tick)
        self.assertEqual(coarse.history, fine.history)

    def test_fixed_scenario_replays_identically(self) -> None:
        def scenario() -> tuple[int, tuple]:
            clock = SimulationClock()
            clock.schedule_at(days(3), "scan", {"target": "asteroid-01"})
            clock.schedule_at(days(3), "telemetry", {"probe": "origin"})
            clock.schedule_at(julian_years(14), "arrival", {"system": "EV-002"})
            clock.advance_to(julian_years(20))
            return clock.tick, clock.history

        self.assertEqual(scenario(), scenario())

    def test_rejects_backward_time(self) -> None:
        clock = SimulationClock(start_tick=100)
        with self.assertRaises(ValueError):
            clock.advance_to(99)
        with self.assertRaises(ValueError):
            clock.schedule_at(99, "past")

    def test_zero_scale_freezes_wall_time_without_marking_paused(self) -> None:
        clock = SimulationClock()
        clock.set_time_scale(0)
        clock.advance_wall_ticks(seconds(5))
        self.assertEqual(clock.tick, 0)
        self.assertFalse(clock.paused)

    def test_wall_tick_scaling_matches_independently_computed_floor_division(self) -> None:
        """Pin the fractional remainder-carry accumulator to a hand-derivable value.

        Every other scaling test in this file compares two separate
        ``SimulationClock`` runs against each other. That style would not catch
        a self-consistent but wrong accumulator (e.g. rounding instead of
        flooring, or losing the carried remainder), because a broken formula
        applied identically on both sides still agrees with itself. This test
        instead checks against ``n * numerator // denominator``, the closed-form
        exact value for an unscaled-then-scaled wall-tick count, computed here
        independently of ``SimulationClock`` internals.
        """
        numerator, denominator = 7, 3
        clock = SimulationClock()
        clock.set_time_scale(numerator, denominator)

        for step in range(1, 301):
            clock.advance_wall_ticks(1)
            expected_tick = step * numerator // denominator
            self.assertEqual(clock.tick, expected_tick, f"mismatch at wall step {step}")

    def test_wall_tick_scaling_matches_closed_form_across_irregular_chunk_sizes(self) -> None:
        """Same independent closed-form check, but with irregular chunk sizes.

        `PROOF.md` claims frame/update-rate independence. The existing test for
        that claim only compares a coarse-chunked run against a fine-chunked
        run, so it inherits the same self-comparison blind spot described
        above. This asserts irregular chunking against the same
        externally-derived exact formula instead.
        """
        numerator, denominator = 22, 7
        clock = SimulationClock()
        clock.set_time_scale(numerator, denominator)

        chunk_sizes = [1, 2, 3, 1, 5, 1, 1, 4, 2, 1, 9, 1, 1, 1, 6]
        cumulative_wall_ticks = 0
        for chunk in chunk_sizes:
            clock.advance_wall_ticks(chunk)
            cumulative_wall_ticks += chunk
            expected_tick = cumulative_wall_ticks * numerator // denominator
            self.assertEqual(clock.tick, expected_tick)

    def test_scheduled_event_rejects_invalid_fields_independently(self) -> None:
        invalid_cases = (
            ({"due_tick": -1, "sequence": 0, "kind": "scan"}, "event due_tick cannot be negative"),
            ({"due_tick": 0, "sequence": -1, "kind": "scan"}, "event sequence cannot be negative"),
            ({"due_tick": 0, "sequence": 0, "kind": ""}, "event kind cannot be empty"),
        )

        for kwargs, message in invalid_cases:
            with self.subTest(kwargs=kwargs):
                with self.assertRaisesRegex(ValueError, message):
                    ScheduledEvent(**kwargs)

    def test_constructor_and_handler_registration_reject_invalid_inputs(self) -> None:
        with self.assertRaisesRegex(ValueError, "start_tick cannot be negative"):
            SimulationClock(start_tick=-1)

        clock = SimulationClock()
        with self.assertRaisesRegex(ValueError, "event kind cannot be empty"):
            clock.register_handler("", lambda _clock, _event: None)

    def test_time_scale_rejects_negative_numerator_and_nonpositive_denominator(self) -> None:
        clock = SimulationClock()

        with self.assertRaisesRegex(ValueError, "time scale cannot be negative"):
            clock.set_time_scale(-1)

        for denominator in (0, -1):
            with self.subTest(denominator=denominator):
                with self.assertRaisesRegex(ValueError, "time scale denominator must be positive"):
                    clock.set_time_scale(1, denominator)

    def test_relative_and_wall_advance_reject_negative_inputs(self) -> None:
        clock = SimulationClock()

        guarded_calls = (
            (lambda: clock.schedule_after(-1, "scan"), "delta_ticks cannot be negative"),
            (lambda: clock.advance_by(-1), "delta_ticks cannot be negative"),
            (lambda: clock.advance_wall_ticks(-1), "wall_ticks cannot be negative"),
        )

        for call, message in guarded_calls:
            with self.subTest(message=message):
                with self.assertRaisesRegex(ValueError, message):
                    call()

    def test_run_until_idle_rejects_negative_max_events(self) -> None:
        clock = SimulationClock()
        with self.assertRaisesRegex(ValueError, "max_events cannot be negative"):
            clock.run_until_idle(max_events=-1)


if __name__ == "__main__":
    unittest.main()
