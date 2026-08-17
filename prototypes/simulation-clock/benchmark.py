"""Simple dependency-free benchmark for the simulation-clock prototype.

This is diagnostic, not a CI performance gate. It intentionally avoids asserting
wall-clock timing because hosted-runner variance would make that brittle.
"""

from __future__ import annotations

from time import perf_counter

from clock import SimulationClock, julian_years


def main() -> None:
    clock = SimulationClock()
    event_count = 100_000
    horizon = julian_years(100_000)

    started = perf_counter()
    for index in range(event_count):
        due_tick = (horizon * (index + 1)) // event_count
        clock.schedule_at(due_tick, "benchmark_event", {"index": index})
    scheduled_at = perf_counter()

    processed = clock.run_until_idle()
    finished = perf_counter()

    print("Everward simulation-clock sparse benchmark")
    print(f"events={event_count}")
    print("horizon_julian_years=100000")
    print(f"processed={processed}")
    print(f"final_tick={clock.tick}")
    print(f"schedule_seconds={scheduled_at - started:.6f}")
    print(f"process_seconds={finished - scheduled_at:.6f}")
    print(f"total_seconds={finished - started:.6f}")


if __name__ == "__main__":
    main()
