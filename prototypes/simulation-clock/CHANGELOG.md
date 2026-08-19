# Simulation Clock Prototype Changelog

## 2026-08-19

- Added two tests that pin the wall-tick rational-scaling accumulator against
  an independently-computed closed-form value (`n * numerator // denominator`)
  instead of only comparing two `SimulationClock` runs against each other.
  Every prior scaling test was self-comparison-only, so a self-consistent but
  wrong remainder-carry formula (for example, rounding instead of flooring)
  would have passed every existing test undetected. No `clock.py` behavior
  changed.

## Initial proof

- deterministic integer simulation time,
- stable scheduled-event ordering,
- rational time acceleration,
- pause semantics,
- frame/update-chunk independence,
- sparse long-duration event advancement,
- headless demonstration trace,
- dependency-free unit tests,
- CI wiring,
- benchmark harness.
