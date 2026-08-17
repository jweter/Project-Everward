"""Run a deterministic headless Everward timing scenario.

Usage:
    python prototypes/simulation-clock/run_demo.py
"""

from __future__ import annotations

import hashlib
import json

from clock import SimulationClock, days, julian_years


def canonical_history(clock: SimulationClock) -> list[dict[str, object]]:
    return [
        {
            "processed_tick": item.processed_tick,
            "sequence": item.sequence,
            "kind": item.kind,
            "payload": dict(item.payload),
        }
        for item in clock.history
    ]


def scenario() -> SimulationClock:
    clock = SimulationClock()

    def on_arrival(sim: SimulationClock, event) -> None:
        sim.schedule_after(days(12), "survey_complete", {"system": event.payload["system"]})
        sim.schedule_after(days(40), "report_transmitted", {"system": event.payload["system"]})

    clock.register_handler("probe_arrival", on_arrival)

    clock.schedule_after(days(3), "asteroid_scan_complete", {"target": "AST-001"})
    clock.schedule_after(days(9), "mining_cycle_complete", {"target": "AST-001"})
    clock.schedule_after(julian_years(14), "probe_arrival", {"probe": "EV-002", "system": "STAR-002"})
    clock.schedule_after(julian_years(32), "message_arrival", {"source": "EV-002"})

    clock.advance_to(julian_years(100))
    return clock


def main() -> None:
    clock = scenario()
    history = canonical_history(clock)
    encoded = json.dumps(history, sort_keys=True, separators=(",", ":")).encode("utf-8")
    digest = hashlib.sha256(encoded).hexdigest()

    print("Everward deterministic simulation-clock proof")
    print(f"final_tick={clock.tick}")
    print(f"processed_events={len(history)}")
    print(f"history_sha256={digest}")
    for item in history:
        print(json.dumps(item, sort_keys=True))


if __name__ == "__main__":
    main()
