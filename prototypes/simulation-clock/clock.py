"""Deterministic simulation clock and event scheduler for Everward.

This prototype intentionally uses only the Python standard library. It proves the
simulation semantics we need before the production engine/runtime is selected.

Core rules:
- authoritative time is an integer tick count;
- one tick is one microsecond for this prototype;
- simultaneous events are ordered by insertion sequence;
- time scaling uses rational arithmetic, never floating point;
- sparse timelines jump directly between scheduled events;
- event handlers may schedule additional events deterministically.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from fractions import Fraction
import heapq
from typing import Any, Callable, Mapping

TICKS_PER_SECOND = 1_000_000
SECONDS_PER_DAY = 86_400
TICKS_PER_DAY = TICKS_PER_SECOND * SECONDS_PER_DAY
DAYS_PER_JULIAN_YEAR = 365.25
TICKS_PER_JULIAN_YEAR = int(TICKS_PER_DAY * DAYS_PER_JULIAN_YEAR)

EventHandler = Callable[["SimulationClock", "ScheduledEvent"], None]


@dataclass(order=True, frozen=True, slots=True)
class ScheduledEvent:
    """A deterministic entry in the simulation event queue."""

    due_tick: int
    sequence: int
    kind: str = field(compare=False)
    payload: Mapping[str, Any] = field(default_factory=dict, compare=False)

    def __post_init__(self) -> None:
        if self.due_tick < 0:
            raise ValueError("event due_tick cannot be negative")
        if self.sequence < 0:
            raise ValueError("event sequence cannot be negative")
        if not self.kind:
            raise ValueError("event kind cannot be empty")


@dataclass(frozen=True, slots=True)
class EventRecord:
    """Canonical record of one processed event."""

    processed_tick: int
    sequence: int
    kind: str
    payload: Mapping[str, Any]


class SimulationClock:
    """Authoritative deterministic time source and sparse event scheduler."""

    def __init__(self, *, start_tick: int = 0) -> None:
        if start_tick < 0:
            raise ValueError("start_tick cannot be negative")

        self._tick = start_tick
        self._queue: list[ScheduledEvent] = []
        self._next_sequence = 0
        self._handlers: dict[str, EventHandler] = {}
        self._paused = False
        self._time_scale = Fraction(1, 1)
        self._scale_remainder = Fraction(0, 1)
        self._history: list[EventRecord] = []

    @property
    def tick(self) -> int:
        return self._tick

    @property
    def paused(self) -> bool:
        return self._paused

    @property
    def time_scale(self) -> Fraction:
        return self._time_scale

    @property
    def pending_event_count(self) -> int:
        return len(self._queue)

    @property
    def history(self) -> tuple[EventRecord, ...]:
        return tuple(self._history)

    def register_handler(self, kind: str, handler: EventHandler) -> None:
        if not kind:
            raise ValueError("event kind cannot be empty")
        self._handlers[kind] = handler

    def set_paused(self, paused: bool) -> None:
        self._paused = bool(paused)

    def set_time_scale(self, numerator: int, denominator: int = 1) -> None:
        if numerator < 0:
            raise ValueError("time scale cannot be negative")
        if denominator <= 0:
            raise ValueError("time scale denominator must be positive")
        self._time_scale = Fraction(numerator, denominator)
        self._scale_remainder = Fraction(0, 1)

    def schedule_at(
        self,
        due_tick: int,
        kind: str,
        payload: Mapping[str, Any] | None = None,
    ) -> ScheduledEvent:
        if due_tick < self._tick:
            raise ValueError("cannot schedule an event in the past")

        event = ScheduledEvent(
            due_tick=due_tick,
            sequence=self._next_sequence,
            kind=kind,
            payload=dict(payload or {}),
        )
        self._next_sequence += 1
        heapq.heappush(self._queue, event)
        return event

    def schedule_after(
        self,
        delta_ticks: int,
        kind: str,
        payload: Mapping[str, Any] | None = None,
    ) -> ScheduledEvent:
        if delta_ticks < 0:
            raise ValueError("delta_ticks cannot be negative")
        return self.schedule_at(self._tick + delta_ticks, kind, payload)

    def peek_next_event(self) -> ScheduledEvent | None:
        return self._queue[0] if self._queue else None

    def advance_to(self, target_tick: int) -> int:
        """Advance to target_tick, processing all due events in canonical order.

        Sparse time is event-driven: the clock jumps to each event timestamp and
        then to target_tick. It never loops once per simulation tick.
        """

        if target_tick < self._tick:
            raise ValueError("cannot move simulation time backwards")

        processed = 0
        while self._queue and self._queue[0].due_tick <= target_tick:
            event = heapq.heappop(self._queue)
            self._tick = event.due_tick
            self._process(event)
            processed += 1

        self._tick = target_tick
        return processed

    def advance_by(self, delta_ticks: int) -> int:
        if delta_ticks < 0:
            raise ValueError("delta_ticks cannot be negative")
        return self.advance_to(self._tick + delta_ticks)

    def advance_wall_ticks(self, wall_ticks: int) -> int:
        """Advance from elapsed wall-clock ticks using exact rational scaling.

        This method proves frame-rate independence. Different wall-step chunking
        produces the same simulation time because fractional scaled ticks are
        carried forward exactly.
        """

        if wall_ticks < 0:
            raise ValueError("wall_ticks cannot be negative")
        if self._paused or self._time_scale == 0:
            return 0

        scaled = Fraction(wall_ticks, 1) * self._time_scale + self._scale_remainder
        simulation_delta = scaled.numerator // scaled.denominator
        self._scale_remainder = scaled - simulation_delta
        return self.advance_by(simulation_delta)

    def run_until_idle(self, *, max_events: int | None = None) -> int:
        """Process queued events until none remain or max_events is reached."""

        if max_events is not None and max_events < 0:
            raise ValueError("max_events cannot be negative")

        processed = 0
        while self._queue and (max_events is None or processed < max_events):
            event = heapq.heappop(self._queue)
            self._tick = event.due_tick
            self._process(event)
            processed += 1
        return processed

    def _process(self, event: ScheduledEvent) -> None:
        self._history.append(
            EventRecord(
                processed_tick=self._tick,
                sequence=event.sequence,
                kind=event.kind,
                payload=dict(event.payload),
            )
        )
        handler = self._handlers.get(event.kind)
        if handler is not None:
            handler(self, event)


def seconds(value: int) -> int:
    return value * TICKS_PER_SECOND


def days(value: int) -> int:
    return value * TICKS_PER_DAY


def julian_years(value: int) -> int:
    return value * TICKS_PER_JULIAN_YEAR
