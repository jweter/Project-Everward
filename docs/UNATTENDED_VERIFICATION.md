# Unattended Verification Policy

Routine engineering verification is machine-owned. The owner is not a recurring test executor and must not be placed inside ordinary implementation, regression, Godot/build, Product Reality, or promotion loops when objective evidence can be automated or collected by an admitted unattended worker. Godot became the active production path on 2026-11-27 (ADR-0014); historical Unreal tests retain their historical evidence status.

## Paid exploratory Aura exception — owner-only

The unattended-machine testing policy applies to **local deterministic C++ and headless Godot tests**, not paid Aura Autopilot sessions. Aura was an owner-only Unreal-era exploratory tool and has no authorized automatic Godot replacement. By owner decision on 2026-11-08, **only Jeremy operates Aura**; no agent, CI, scheduled job, desktop automation, or unattended worker may launch, control, schedule, retry, or automatically consume Aura credits. Completed owner-run Aura reports become evidence for agents to convert into scoped defects, roadmap slices and free/local regression tests. Do not ask Jeremy to rerun Aura for every fix or mark independent development blocked until he elects a future exploratory run. See `docs/integrations/OWNER_RUN_AURA_REPORT_TO_ROADMAP.md`.

## Default verification path

`change -> FAST_GATE -> exact-head preflight -> CI -> unattended Godot/environment verification -> structured evidence -> automated diagnosis/repair -> re-verification -> promotion -> optional Jeremy milestone acceptance`

Environment dependence is not automatically human dependence. Native C++ builds, headless Godot parity/first-loop checks, applicable Godot packaged-launch checks, simulation probes, save/load verification and controls/HUD checks should first be attempted by an approved unattended worker on an idle machine. Never silently bypass resource admission, explicit owner control or exact-commit checks.

## Evidence classification

Use this order:

1. Repository-native deterministic test.
2. Unattended Godot/environment test on the exact commit (distinguish `GODOT_VERIFIED` from human art/game-feel acceptance).
3. Automatable Product Reality probe using logs, automation reports, screenshots, traces, save files, telemetry, deterministic state snapshots, launch/build results, or other machine-readable observations.
4. Human judgment only when the remaining question is genuinely subjective, ambiguous, irreversible, safety-sensitive, or a gameplay/product-direction decision.

`Jeremy must run this manually` is a workflow deficiency unless category 4 actually applies.

## Worker contract

An unattended worker must identify repository, branch, exact SHA, request, engine/build configuration, environment, and timestamp; record bounded commands/actions and evidence; return `PASS`, `FAIL`, `REVIEW_REQUIRED`, `PRODUCT_REALITY_REQUIRED`, or `ENVIRONMENT_FAILURE`; write permitted results back to the issue/PR/evidence ledger; fail closed on missing evidence; preserve secrets/private files; and leave Godot/editor workloads and the machine in a safe state.

Pending worker verification blocks only the dependent lane. Deterministic simulation, interaction/manipulator/resource/mining/repair foundations, save/load, controls/HUD architecture, component/damage/recovery work, and test infrastructure continue when independent.

## Everward application — active Godot path

Objective checks such as native C++ compilation, Godot GDExtension/native parity, exact-head `--firstloop` tests, deterministic fixed-step behavior, save/load round trips, component, resource, mining and repair invariants, controls/HUD checks, crash detection, screenshots and reproducible telemetry should run through CI or an authorized unattended worker where feasible. Treat **missing or invalid assertions** as failures even if a process exits zero. Track the known first-GDExtension-import crash separately; successful import-cache creation does not establish passing runtime behavior.

**Legacy Unreal worker history:** the earlier Windows worker used UBT editor builds, headless Unreal smoke and bounded low-spec startup/memory checks. Those historic results are not Godot verification and do not grant current worker authority. The Godot worker path, packaged-game testing and full player interaction capture require explicit current implementation and evidence before being marked operational.

Human review remains appropriate for irreducibly subjective gameplay feel, visual quality, pacing, artistic presentation, or product-direction decisions. Even then, automate setup, navigation, evidence capture, and regression checks so Jeremy evaluates only the final subjective question.

The owner may always play a milestone or new version, but repeated incremental Godot tests should not be required for development to continue.
