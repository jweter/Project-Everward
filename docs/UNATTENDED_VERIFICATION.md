# Unattended Verification Policy

Routine engineering verification is machine-owned. Jeremy is not a recurring test executor and must not be placed inside ordinary implementation, regression, Unreal/build, Product Reality, or promotion loops when objective evidence can be automated or collected by an unattended worker.

## Default verification path

`change -> FAST_GATE -> exact-head preflight -> CI -> unattended Unreal/environment verification -> structured evidence -> automated diagnosis/repair -> re-verification -> promotion -> optional Jeremy milestone acceptance`

Environment dependence is not automatically human dependence. UBT builds, Unreal automation, packaged launch checks, simulation probes, save/load verification, controls/HUD checks, and other non-CI work should first be scheduled to an approved unattended worker on an idle machine.

## Evidence classification

Use this order:

1. Repository-native deterministic test.
2. Unattended Unreal/environment test.
3. Automatable Product Reality probe using logs, automation reports, screenshots, traces, save files, telemetry, deterministic state snapshots, launch/build results, or other machine-readable observations.
4. Human judgment only when the remaining question is genuinely subjective, ambiguous, irreversible, safety-sensitive, or a gameplay/product-direction decision.

`Jeremy must run this manually` is a workflow deficiency unless category 4 actually applies.

## Worker contract

An unattended worker must identify repository, branch, exact SHA, request, engine/build configuration, environment, and timestamp; record bounded commands/actions and evidence; return `PASS`, `FAIL`, `REVIEW_REQUIRED`, `PRODUCT_REALITY_REQUIRED`, or `ENVIRONMENT_FAILURE`; write permitted results back to the issue/PR/evidence ledger; fail closed on missing evidence; preserve secrets/private files; and leave Unreal and the machine in a safe state.

Pending worker verification blocks only the dependent lane. Deterministic simulation, interaction/manipulator/resource/mining/repair foundations, save/load, controls/HUD architecture, component/damage/recovery work, and test infrastructure continue when independent.

## Everward application

Objective checks such as UBT compilation, Unreal Automation tests, packaged startup, deterministic fixed-step behavior, save/load round trips, component/system state, resource/mining/repair invariants, input-state transitions, HUD data contracts, crash detection, screenshots, and reproducible telemetry should move into CI or the unattended worker.

The unattended Windows worker's low-spec startup check records bounded editor startup and process working-set facts for the exact tested commit. Missing or failed telemetry stays `REVIEW_REQUIRED`/`FAIL`, and a passing check is never converted into visual, control-feel, frame-time, or gameplay Product Reality (see `docs/UNATTENDED_WINDOWS_PRODUCT_REALITY.md`).

Human review remains appropriate for irreducibly subjective gameplay feel, visual quality, pacing, artistic presentation, or product-direction decisions. Even then, automate setup, navigation, evidence capture, and regression checks so Jeremy evaluates only the final subjective question.

Jeremy may always play a milestone or new version, but repeated incremental Unreal tests should not be required for development to continue.
