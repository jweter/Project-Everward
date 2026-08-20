# Everward Project Status

This file is the durable operational continuation record for human and scheduled development. It records where active work should resume without replacing the authoritative roadmap, design pillars, accepted ADRs, engine direction, or architecture documents.

## Current phase

**Phase 1 — Technical Proofs**

Phase 0 project-constitution material is established. Phase 2 — One Probe is not authorized until the Phase 1 exit gate passes with real evidence.

## Accepted production direction

**Unreal Engine is the accepted production engine direction.**

The engine decision is no longer an open Godot-versus-Unreal product choice. The authoritative decision is recorded in:

- `ENGINE_DIRECTION.md`
- `TECHNOLOGY_DECISIONS.md` TD-001
- `DECISION_LOG.md` ADR-0001

Godot prototype material remains only as comparative Phase 1 benchmark evidence and historical technical work. It must not be interpreted by automation as an alternate authorized production path.

## Current milestone / gate

Complete the Phase 1 technical-proof evidence set and satisfy `PHASE1_EXIT_GATE.md`:

- deterministic simulation clock;
- deterministic procedural star-system proof;
- representative rendering benchmark;
- massive-coordinate handling proof;
- headless long-duration simulation;
- real decision-ready engine artifact validating **Unreal Engine** under the current project constraints.

The executable gate in `prototypes/phase1_exit_gate.py` requires the recommendation to be `unreal`.

## Current blocker

All five Phase 1 prototype areas exist and their automated foundations are established. The remaining Phase 1 blocker is the real hardware rendering evidence required to validate Unreal and expose any material Unreal-specific blocker before Phase 2 production gameplay begins.

The existing Godot/Unreal comparison tooling may still be used to collect comparative evidence, but a Godot recommendation does not authorize Phase 2. Under ADR-0001 it means either:

1. investigate and resolve the Unreal blocker, or
2. explicitly supersede the accepted Unreal decision with a new ADR documenting the consequences for Everward's visual promise.

No automated process may silently choose Godot because it benchmarks lighter, faster, or easier.

## Exact continuation point

The next human-assisted Phase 1 task is to complete representative hardware capture/evidence for the rendering benchmark on the development machine and produce a real `decision_ready` artifact that validates Unreal.

Until that evidence exists, automation should:

1. inspect open PRs and CI first;
2. repair failed existing work before starting new work;
3. merge only independently verified GREEN and fully merge-ready PRs;
4. remain within authorized Phase 1 work;
5. fix concrete regressions, stale contracts, documentation drift, or evidence-pipeline defects when found;
6. avoid inventing marginal Phase 1 scope merely because Phase 2 is hardware-gated;
7. never begin Phase 2 production gameplay until `prototypes/phase1_exit_gate.py` passes with Unreal validation.

Most recent slice (2026-08-20): extended the per-field/per-key coverage audit **outside** `prototypes/rendering-benchmark/` for the first time. The preceding eleven closures all sat in the engine-decision evidence pipeline, leaving the four core simulation prototypes unaudited for that gap class. `prototypes/coordinate-scale` — the massive-coordinate proof required by `PHASE1_EXIT_GATE.md`, and the thinnest-tested prototype in the repository — had its `offset_mm` normalization rule exercised on the x axis only (and only at the upper bound) and its three-axis shape rule not exercised at all, in any of `SpatialPosition`, `from_total_mm()`, or `translated_mm()`. Named the implicit contract as `AXIS_COUNT`/`VECTOR_FIELDS` constants (behavior-preserving) and added per-axis, per-field, and pinned-declared-set tests; `coordinate-scale` now runs 13 tests, up from 6. Verified by four independent mutations, each reverted byte-identical. See `ERROR_RESOLUTION_LEDGER.md` for detail.

Remaining named coverage-gap candidates, in rough descending value, are the three core simulation prototypes still unaudited for this gap class: `simulation-clock` (`clock.py`'s guard clauses — `ScheduledEvent.__post_init__`'s three rejections, `register_handler`'s empty-kind rejection, and the negative-argument rejections on `set_time_scale`, `schedule_after`, `advance_by`, `advance_wall_ticks`, and `run_until_idle` — are largely unexercised by `test_clock.py`'s 11 behavior-focused tests), then `headless-simulation` and `procedural-system`. These are ordinary Phase 1 prototype-quality work under `TESTING_STRATEGY.md`; none of them unblocks the gate.

The next Phase 1 continuation point remains the human-assisted hardware rendering-benchmark evidence capture described above, which is the only work that actually advances the exit gate.

When the gate passes, the continuation point becomes:

> **Phase 2 — One Probe in Unreal Engine.**

## Phase 2 production rule

Once authorized, production-facing gameplay, presentation, asset planning, rendering, HUD integration, and local cinematic implementation should target Unreal Engine.

The simulation architecture remains engine-independent in principle:

- simulation owns mechanical truth;
- Unreal consumes authoritative simulation state;
- deterministic headless execution remains required;
- save data remains a versioned schema rather than blind Unreal object serialization;
- large-scale simulation work must not become inseparable from rendered actors/components.

## Visual product constraint

Everward must not drift toward a primarily 2D, 2.5D, abstract-map, low-poly, deliberately quirky, or visually lightweight interpretation merely because it is easier to implement.

The target is cinematic, immersive, high-fidelity 3D scientific realism. The player is the probe, and physical presence in a universe worth looking at is a first-class product requirement.

## Automation operating state

Scheduled development is governed by `AGENT_DEVELOPMENT_POLICY.md`.

Every hourly run should:

1. inspect open PRs and CI first;
2. repair failed existing work before new roadmap work;
3. merge only work that is independently verified green and fully merge-ready;
4. otherwise advance one highest-value authorized slice of the current roadmap phase;
5. keep affected documentation current;
6. use accepted ADRs and this status file to avoid reopening settled decisions.

## Repository posture

- Repository visibility: **Public by deliberate operational choice**.
- Project IP posture: **Proprietary, all rights reserved**.
- Public visibility does not grant an open-source license.
- Default branch: `main`.
- Substantive autonomous development: branch + pull request; no direct-to-main development.

## Historical technical evidence

Detailed Phase 1 regression discoveries, fixes, mutation-test evidence, and root-cause history belong in the prototype-specific proof/changelog files and `ERROR_RESOLUTION_LEDGER.md` rather than accumulating indefinitely in this operational status document.

This file should remain concise and current: **where we are, what blocks us, what decision is settled, and what work is authorized next.**