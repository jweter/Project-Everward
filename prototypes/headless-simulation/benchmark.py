from __future__ import annotations

from time import perf_counter
import tracemalloc

from runner import HeadlessSimulation


def main() -> None:
    years = 10_000
    seed = 847291

    tracemalloc.start()
    start = perf_counter()
    summary = HeadlessSimulation(seed=seed, years=years).run()
    elapsed = perf_counter() - start
    _, peak_bytes = tracemalloc.get_traced_memory()
    tracemalloc.stop()

    events_per_second = summary.processed_events / elapsed if elapsed else float("inf")
    print(f"simulated_years={years}")
    print(f"processed_events={summary.processed_events}")
    print(f"elapsed_seconds={elapsed:.6f}")
    print(f"events_per_second={events_per_second:.2f}")
    print(f"peak_tracemalloc_bytes={peak_bytes}")
    print(f"fingerprint={summary.fingerprint}")


if __name__ == "__main__":
    main()
