# Simulation Clock Prototype

## Purpose

Prove deterministic simulation time, pause/resume, scheduled events, and aggressive time acceleration independently of rendering.

## Minimum proof

- authoritative simulation time,
- deterministic event queue,
- stable ordering for simultaneous events,
- pause/resume,
- multiple time scales,
- very long future events,
- canonical result independent of rendered frame rate.

## Acceptance criteria

A fixed scenario can run repeatedly and produce the same final state. Long sparse timelines advance by scheduled work rather than by naively iterating every rendered frame.

This prototype is expected to become the first serious implementation PR after project foundation.
