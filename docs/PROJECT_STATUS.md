# Everward Project Status

This file is the durable operational continuation record for human and scheduled development. It records where active work should resume without replacing the authoritative roadmap, design pillars, accepted ADRs, engine direction, or architecture documents.

## Current phase

**Phase 1 — Technical Proofs: exit gate satisfied. Phase 2 — One Probe is authorized but not yet started.**

Phase 0 project-constitution material is established. `prototypes/phase1_exit_gate.py` now passes against a real, merged `decision_ready` Unreal evidence artifact (PR #63, merged 2026-08-21), so Phase 2 — One Probe is authorized by the roadmap's own exit condition. No Phase 2 implementation work has begun: `src/` remains the placeholder described in `src/README.md`, and standing up actual Phase 2 production work is a substantial new undertaking, not something this reconciliation captures. See "Exact continuation point" below.

## Accepted production direction

**Unreal Engine is the accepted production engine direction.**

The engine decision is no longer an open Godot-versus-Unreal product choice. The authoritative decision is recorded in:

- `ENGINE_DIRECTION.md`
- `TECHNOLOGY_DECISIONS.md` TD-001
- `DECISION_LOG.md` ADR-0001

Godot prototype material remains only as comparative Phase 1 benchmark evidence and historical technical work. It must not be interpreted by automation as an alternate authorized production path.

## Current milestone / gate

The Phase 1 technical-proof evidence set required by `PHASE1_EXIT_GATE.md` is now complete:

- deterministic simulation clock — present;
- deterministic procedural star-system proof — present;
- representative rendering benchmark — present, with a real hardware capture;
- massive-coordinate handling proof — present;
- headless long-duration simulation — present;
- real decision-ready engine artifact validating **Unreal Engine** under the current project constraints — present (`prototypes/rendering-benchmark/captures/phase1-engine-decision.json`, `status: decision_ready`, `recommendation: unreal`, captured 2026-08-21).

Running the executable gate confirms this directly:

```
python prototypes/phase1_exit_gate.py \
  --engine-decision prototypes/rendering-benchmark/captures/phase1-engine-decision.json
```

returns `"blockers": []`, `"phase1_complete": true`, `"phase2_one_probe_authorized": true`.

## Current blocker

**None for the Phase 1 gate itself.** All five Phase 1 prototype areas exist, and the real Unreal hardware evidence PR #63 was looking for now exists and is decision-ready. The gate no longer blocks Phase 2.

The measured evidence in `prototypes/rendering-benchmark/captures/unreal-run-record.json` (Intel Iris Xe programming laptop, 2560x1440) recorded and explicitly logged, rather than hid, real residual risk that Phase 2 work should account for:

- GPU frame time 61.63 ms, missing the 60 FPS / ~16.7 ms target on this hardware (strongly GPU-bound);
- the 2560x1440 output was upscaled from an internal 1538x887 (60.3%) render resolution rather than rendered natively;
- the captured scene was extremely dark and not every required visual shell was clearly distinguishable in the screenshots;
- a stronger gaming-PC capture remains useful follow-up evidence once that hardware is available.

PR #63 (authored and merged by the repository owner) treated these as accepted production-quality risk to track in Phase 2 rather than an Unreal-specific blocker under ADR-0001, and marked the artifact `decision_ready` on that basis. This status file records that judgment; it does not relitigate it. Godot recommendations remain non-authorizing for Phase 2 per ADR-0001, and no automated process may silently choose Godot because it benchmarks lighter, faster, or easier.

## Exact continuation point

**The Phase 1 exit gate now passes with real evidence. The continuation point is:**

> **Phase 2 — One Probe in Unreal Engine** (see `ROADMAP.md`).

Phase 2 implements one embodied machine in Unreal with mass, energy, storage, sensors, computation, propulsion, position, velocity, temperature, component capabilities, and software state, with the player able to observe, scan, move, inspect systems, manage power, and alter basic software policies, while the simulation core (not the Unreal layer) remains the owner of mechanical truth per ADR-0002 and `ARCHITECTURE.md`.

Actually standing up Phase 2 — a real Unreal project, promoting/adapting simulation-core code out of `prototypes/` into `src/` per `ARCHITECTURE.md`, and implementing the One Probe embodiment — is a substantial new-architecture undertaking, not a single small slice. It intentionally was not started as part of this reconciliation update, consistent with `AGENT_DEVELOPMENT_POLICY.md` §5's one-substantial-slice-per-run budget and its preference for finishing/reconciling over starting speculative work. A future run should treat defining the Phase 2 kickoff slice (e.g. initial Unreal project scaffold and the first authoritative-state/presentation boundary) as its own authorized piece of work, informed by `ARCHITECTURE.md`, `SAVE_FORMAT.md`, and the residual visual/performance risk logged above.

Until Phase 2 work actually begins, automation should still:

1. inspect open PRs and CI first;
2. repair failed existing work before starting new work;
3. merge only independently verified GREEN and fully merge-ready PRs;
4. fix concrete regressions, stale contracts, documentation drift, or evidence-pipeline defects when found;
5. keep this status file current with the real state of the gate and the roadmap phase.

Most recent slices:

- (2026-08-21) **Status reconciliation:** `docs/PROJECT_STATUS.md` had drifted stale relative to `main`. PR #63 (merged 2026-08-21 20:53 UTC) published the real Unreal hardware evidence and made the Phase 1 exit gate pass, but PR #62 (merged 2026-08-21 20:55 UTC, branched from before #63) rewrote this same file from its own pre-#63 branch state and won the last-write race, so the merged file kept saying Phase 1 was still blocked on hardware evidence that had, in fact, already landed. This update reconciles the file with the real, currently-passing gate. See `ERROR_RESOLUTION_LEDGER.md` for detail.
- (2026-08-21) extended the per-field/per-key coverage audit to `prototypes/procedural-system`. Its two `generate_system()` entry guards (a coordinate not carrying exactly three axes; a non-positive `generator_version`) and the three sampling-method guards on `DeterministicStream` (`below`'s non-positive `upper_exclusive`, `between`'s `high < low`, `weighted`'s non-positive total weight) now have direct regression coverage for every declared branch, confirmed by mutation. This was the last prototype named as a coverage-gap candidate for this pattern, following the coordinate-scale, simulation-clock, and headless-simulation audits; all five Phase 1 prototype areas now have this class of guard-clause coverage.

There is no further named coverage-gap candidate of this specific pattern remaining across the five Phase 1 prototype areas. Future ordinary-quality slices in the Phase 1 prototype areas should look for concrete defects, stale contracts, or documentation drift rather than assuming another prototype still needs this exact audit.

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
