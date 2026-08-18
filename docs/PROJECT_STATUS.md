# Everward Project Status

This file is the operational continuation record for human and scheduled development. It records where active work should resume without replacing the authoritative roadmap, design pillars, accepted ADRs, or architecture documents.

## Current phase

**Phase 1 — Technical Proofs**

Phase 0 project-constitution material is established. Phase 2 — One Probe is not authorized until the Phase 1 exit gate passes with evidence.

## Current milestone / gate

Complete the Phase 1 technical-proof evidence set and satisfy `PHASE1_EXIT_GATE.md`:

- deterministic simulation clock;
- deterministic procedural star-system proof;
- representative space-rendering benchmark;
- massive-coordinate handling proof;
- headless long-duration simulation;
- evidence-backed engine decision packet recommending Godot or Unreal and marked decision-ready.

## Current blockers

The Phase 1 exit gate remains authoritative. Missing or insufficient rendering/engine evidence, a non-decision-ready engine packet, or any missing required technical proof blocks Phase 2 production gameplay.

## Automation operating state

Everward is being prepared for scheduled hourly autonomous development under `AGENT_DEVELOPMENT_POLICY.md`.

Every hourly run should:

1. inspect open PRs and CI first;
2. repair failed existing work before starting new work;
3. merge only work that is independently verified green and fully merge-ready;
4. otherwise advance one highest-value authorized slice of the current roadmap phase;
5. keep affected documentation current;
6. update this file when the continuation point materially changes.

## Repository posture

- Repository visibility: **Public by deliberate operational choice**.
- Project IP posture: **Proprietary, all rights reserved**.
- Public visibility does not grant an open-source license.
- Default branch: `main`.
- Substantive autonomous development: branch + pull request; no direct-to-main development.

## Exact continuation point

All five Phase 1 prototype directories exist, and every prototype's automated test suite is green, including `prototypes/test_phase1_exit_gate.py`, which is now discovered and run by `Foundation checks` CI (previously it existed and passed locally but had no CI step, so a regression in the gate script itself could have merged unverified; see `ERROR_RESOLUTION_LEDGER.md`, 2026-08-18).

The remaining Phase 1 blocker is the rendering-benchmark decision artifact itself: Prototype C's tooling (`prototypes/rendering-benchmark/`) is fully built and tested, but `prototypes/rendering-benchmark/finalize_engine_decision.py` still needs a real, decision-ready artifact produced from actual Godot and Unreal captures on the same physical hardware per `CAPTURE_RUNBOOK.md`. That capture is a physical-hardware/engine-execution task and cannot be produced by a headless automated run.

Until that hardware evidence exists, hourly automation should: verify no open PR/issue needs attention; re-run the full local test/foundation suite to confirm nothing regressed; and, if it finds one, close a further small, concretely evidenced gap in the Phase 1 tooling or its documentation (test coverage, stale contract, drifted fixture, etc.) rather than inventing new roadmap scope. Do not begin Phase 2 — One Probe until the exit gate actually passes with a real `decision_ready` artifact.

## Maintenance rule

Update this document in the same PR whenever a change materially alters:

- current phase;
- current milestone/exit gate;
- active blocking work;
- automation operating state;
- repository posture relevant to development;
- exact continuation point.

Historical rationale belongs in `DECISION_LOG.md`; significant failure history belongs in `ERROR_RESOLUTION_LEDGER.md`.
