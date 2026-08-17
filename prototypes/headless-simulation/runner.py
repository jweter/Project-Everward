"""Headless long-horizon simulation proof for Everward.

This prototype deliberately contains no rendering or engine dependency. It composes
Phase 1's deterministic simulation clock with a tiny periodic workload so we can
prove long-horizon execution, replay, checkpoint/restore, and canonical summaries.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass
import hashlib
import importlib.util
import json
from pathlib import Path
from typing import Any


_CLOCK_PATH = Path(__file__).resolve().parents[1] / "simulation-clock" / "clock.py"
_SPEC = importlib.util.spec_from_file_location("everward_simulation_clock", _CLOCK_PATH)
if _SPEC is None or _SPEC.loader is None:  # pragma: no cover - environment guard
    raise RuntimeError(f"unable to load simulation clock from {_CLOCK_PATH}")
_CLOCK_MODULE = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(_CLOCK_MODULE)

SimulationClock = _CLOCK_MODULE.SimulationClock
TICKS_PER_JULIAN_YEAR = _CLOCK_MODULE.TICKS_PER_JULIAN_YEAR

_PERIOD_YEARS = {
    "maintenance": 1,
    "survey": 10,
    "archive": 100,
}


@dataclass(slots=True)
class HeadlessState:
    maintenance_cycles: int = 0
    survey_cycles: int = 0
    archive_cycles: int = 0
    deterministic_accumulator: int = 0

    @property
    def processed_events(self) -> int:
        return self.maintenance_cycles + self.survey_cycles + self.archive_cycles


@dataclass(frozen=True, slots=True)
class HeadlessSnapshot:
    seed: int
    horizon_tick: int
    current_tick: int
    state: dict[str, int]
    next_due: dict[str, int]

    def canonical_json(self) -> str:
        return json.dumps(asdict(self), sort_keys=True, separators=(",", ":"))

    @classmethod
    def from_json(cls, payload: str) -> "HeadlessSnapshot":
        raw = json.loads(payload)
        return cls(
            seed=int(raw["seed"]),
            horizon_tick=int(raw["horizon_tick"]),
            current_tick=int(raw["current_tick"]),
            state={str(key): int(value) for key, value in raw["state"].items()},
            next_due={str(key): int(value) for key, value in raw["next_due"].items()},
        )


@dataclass(frozen=True, slots=True)
class HeadlessSummary:
    seed: int
    requested_years: int
    final_tick: int
    processed_events: int
    pending_events: int
    maintenance_cycles: int
    survey_cycles: int
    archive_cycles: int
    deterministic_accumulator: int
    fingerprint: str

    def canonical_json(self) -> str:
        return json.dumps(asdict(self), sort_keys=True, separators=(",", ":"))


class HeadlessSimulation:
    """Small deterministic workload used to prove renderer-independent execution."""

    def __init__(self, *, seed: int, years: int) -> None:
        if years <= 0:
            raise ValueError("years must be positive")

        self.seed = int(seed)
        self.requested_years = int(years)
        self.horizon_tick = self.requested_years * TICKS_PER_JULIAN_YEAR
        self.clock = SimulationClock()
        self.state = HeadlessState()
        self.next_due: dict[str, int] = {}
        self._register_handlers()
        self._schedule_initial_events()

    @classmethod
    def from_snapshot(cls, snapshot: HeadlessSnapshot) -> "HeadlessSimulation":
        if snapshot.horizon_tick <= snapshot.current_tick:
            raise ValueError("snapshot must have simulation time remaining")
        if snapshot.horizon_tick % TICKS_PER_JULIAN_YEAR != 0:
            raise ValueError("snapshot horizon must align to whole Julian years")

        simulation = cls.__new__(cls)
        simulation.seed = snapshot.seed
        simulation.horizon_tick = snapshot.horizon_tick
        simulation.requested_years = snapshot.horizon_tick // TICKS_PER_JULIAN_YEAR
        simulation.clock = SimulationClock(start_tick=snapshot.current_tick)
        simulation.state = HeadlessState(**snapshot.state)
        simulation.next_due = dict(snapshot.next_due)
        simulation._register_handlers()
        for kind, due_tick in sorted(simulation.next_due.items()):
            if due_tick <= simulation.horizon_tick:
                simulation.clock.schedule_at(due_tick, kind)
        return simulation

    def _register_handlers(self) -> None:
        for kind in _PERIOD_YEARS:
            self.clock.register_handler(kind, self._handle_periodic_event)

    def _schedule_initial_events(self) -> None:
        for kind, period_years in _PERIOD_YEARS.items():
            due_tick = period_years * TICKS_PER_JULIAN_YEAR
            self.next_due[kind] = due_tick
            if due_tick <= self.horizon_tick:
                self.clock.schedule_at(due_tick, kind)

    def _handle_periodic_event(self, clock: Any, event: Any) -> None:
        kind = event.kind
        if kind == "maintenance":
            self.state.maintenance_cycles += 1
            ordinal = self.state.maintenance_cycles
        elif kind == "survey":
            self.state.survey_cycles += 1
            ordinal = self.state.survey_cycles
        elif kind == "archive":
            self.state.archive_cycles += 1
            ordinal = self.state.archive_cycles
        else:  # pragma: no cover - registration keeps this unreachable
            raise ValueError(f"unknown headless event kind: {kind}")

        digest = hashlib.sha256(
            f"{self.seed}:{kind}:{ordinal}:{clock.tick}".encode("utf-8")
        ).digest()
        self.state.deterministic_accumulator ^= int.from_bytes(digest[:8], "big")

        next_due = clock.tick + _PERIOD_YEARS[kind] * TICKS_PER_JULIAN_YEAR
        self.next_due[kind] = next_due
        if next_due <= self.horizon_tick:
            clock.schedule_at(next_due, kind)

    def advance_to_year(self, year: int) -> int:
        if year < 0 or year > self.requested_years:
            raise ValueError("year must be inside the configured horizon")
        return self.clock.advance_to(year * TICKS_PER_JULIAN_YEAR)

    def snapshot(self) -> HeadlessSnapshot:
        return HeadlessSnapshot(
            seed=self.seed,
            horizon_tick=self.horizon_tick,
            current_tick=self.clock.tick,
            state={
                "maintenance_cycles": self.state.maintenance_cycles,
                "survey_cycles": self.state.survey_cycles,
                "archive_cycles": self.state.archive_cycles,
                "deterministic_accumulator": self.state.deterministic_accumulator,
            },
            next_due=dict(self.next_due),
        )

    def run(self) -> HeadlessSummary:
        self.clock.advance_to(self.horizon_tick)
        return self.summary()

    def summary(self) -> HeadlessSummary:
        payload = {
            "seed": self.seed,
            "requested_years": self.requested_years,
            "final_tick": self.clock.tick,
            "processed_events": self.state.processed_events,
            "pending_events": self.clock.pending_event_count,
            "maintenance_cycles": self.state.maintenance_cycles,
            "survey_cycles": self.state.survey_cycles,
            "archive_cycles": self.state.archive_cycles,
            "deterministic_accumulator": self.state.deterministic_accumulator,
        }
        canonical = json.dumps(payload, sort_keys=True, separators=(",", ":"))
        fingerprint = hashlib.sha256(canonical.encode("utf-8")).hexdigest()
        return HeadlessSummary(**payload, fingerprint=fingerprint)
