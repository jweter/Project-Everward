# Phase 2 — nonblocking authoritative playtest event bridge

**Status 2026-11-08:** UE C++ instrumentation source written; exact-head CI/UE5.8 build and actual PIE/packaged gameplay evidence pending. No Product Reality or user-visible milestone is claimed.

## Motivation

PR #369 establishes a fail-closed future autonomous playtest evidence contract. However, main's recorder only emits some actual events: Fix_It recorded scan/repair-related events in its own actor, while the simulation's scan completion and mining's accepted material transfer were not automatically written as structured events. This pilot records those mechanical facts directly from their existing authoritative source, while leaving the simulation and gameplay commands unchanged.

## Changes

- `UProbeSimulationAdapter::RecordSimulationPlaytestEvent`: optional and nonblocking bridge to the existing `APlaytestRecorderActor`. Missing recorder is a no-op. Does not create actors or modify simulation state.
- `TickComponent`: after draining *real* simulation domain events, writes `scan_completed` or `scan_cancelled` with authoritative simulation tick and domain-event details. A scan start command cannot produce a completion event.
- `CommandMineBootstrapTarget`: only after `MiningSystem::mine_once()` returns `accepted`, `Core->add_stored_material_kg()` succeeds, and actual `Core` storage increases, writes `mining_reach_verified` and `material_collected`. Includes target, actual choosing arm, measured tip/surface gap, permitted reach, material ID, actual mass delta, simulation tick. Rejected mining attempts never emit a success marker.
- Python source-contract regression tests make accidental relocation of these events before a successful command detectable in portable CI. This is **not** a UE functional test and cannot certify runtime integration.

## Important gameplay-contact finding

The engine-independent `MiningSystem::kGeneration1ToolWorkingReachM` permits extraction with tool surface gap up to **0.55 m**. Successful extraction therefore does not necessarily mean touching the asteroid mesh. We intentionally call the event `mining_reach_verified`, not `mining_contact`. The latter must remain a separately measured, strict geometry/visual agreement check before the Phase-2 gameplay gate can pass.

Also preserve distinction between authoritative geometry collision (a spherical sample for tool tip and asteroid body) and artistic mesh alignment (Unreal); accepting a command and seeing text is insufficient evidence for what the player perceives. Do not lower tolerance just to make test green without gameplay design/physics validation.

## Evidence flow and staged validation

```text
Real scanned target -> engine-owned DomainEventType::ScanCompleted
  -> optional Unreal recorder scan_completed (not start_scan command return)
Real tool and body positions -> MiningSystem::mine_once
  -> accepted AND authoritative storage delta positive
  -> mining_reach_verified with numeric surface gap
  -> material_collected with material_id and actual before/after kg
  -> later FixItRuntimeActor repair lifecycle and material/energy consumption
```

Expected existing raw session paths: `unreal/Saved/Playtests/<session>/events.jsonl` with `session.json` and `telemetry.csv`.

1. Portable: run `python -m unittest discover -s tools -p 'test_authoritative_playtest_event_bridge.py' -v` and full repository preflight. Check exact branch/head; source review must also consider Unreal C++ compilation.
2. Unattended UE5.8: use a **dedicated test checkout** under the existing owner/idle/lock controls to compile, load the starter map and capture an actual complete recorder session. This PR does not change the unattended worker or schedule.
3. Native functional test: assert target selection, scan complete, measured gap against the exact tool/mesh, a positive per-material inventory delta, material+energy spending by Fix_It and increased component integrity. This Unreal assertion producer is NOT implemented by this PR.
4. Negative regression: insufficient sensor power, unscanned deposit, undeployed/no-tool arm, over-reach, penetrative geometry, storage full and an interrupted repair must not produce a fraudulent success chain.
5. Connect to `tools/playtest/agent_session_gate.py` from draft PR #369 only after its independent native C++/Blueprint/Automation assertion file and exact-SHA run context are generated. The scenario's `mining_contact` must not be substituted with `mining_reach_verified` or an AI-generated statement.
6. Compare real screenshots/telemetry at Emergency and Low presets and measure CPU/RAM; geometry or visual truth is NEVER inferred from a green source-only test.

## Safety and scope

No changes to main, Unreal project plugin configuration, AI accounts/API spending, scheduled workers, Iron God owner-control modes, save schema, input bindings, fixed-step physics, mining reach constant, material balance or Fix_It planner. Runtime recorder omission is still best effort; it does not affect whether mining succeeds. Roll back by reverting this PR.

## Claude handoff

Read AGENTS.md, current main and Product Reality evidence before cherry-picking or integrating. Review source hooks for editor/UHT compatibility and actual compile (CI's portable source-contract test is not enough). Connect to PR #369's independent evidence contract only with true runtime/functional tests and exact build identity. On failure, report real logs; do not invent contact or fabricate successful `authoritative_assertions.json`.