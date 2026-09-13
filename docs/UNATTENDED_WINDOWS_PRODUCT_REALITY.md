# Unattended Windows Product Reality Worker

Everward's Windows laptop should remain a real Unreal/GPU verification machine without requiring a human to repeatedly perform deterministic QA labor.

The governing rule is:

> **Zero humans for facts a computer can measure. Human attention is reserved for experience.**

This document defines the first implementation slice for issue #240.

## What the first worker does

The first unattended slice is intentionally conservative. It proves engineering reality on the actual Windows/Unreal machine without claiming that the game is visually or experientially correct.

After the signed-in Windows user is idle for the configured interval, the worker:

1. acquires a local single-run lock;
2. records the exact clean Git commit under test;
3. records CPU/GPU identity when Windows exposes it;
4. runs `python tools/quality_preflight.py --full`;
5. discovers Unreal Engine 5.8 using the same environment/registry/common-path rules as the Phase-2 harness;
6. builds `EverwardEditor Win64 Development` through UnrealBuildTool;
7. captures exit codes, durations, and a sanitized failure tail;
8. writes evidence under `%LOCALAPPDATA%\Everward\unattended-worker\`;
9. exits with PASS, FAIL, or REVIEW_REQUIRED.

The repository is not modified by a run. Worker state is local-only.

## Result semantics

### PASS

PASS means:

- the repository was a clean exact commit;
- the full canonical preflight passed;
- Unreal Engine 5.8 was found;
- `EverwardEditor Win64 Development` built successfully.

PASS **does not** mean the game is fun, visually correct, playable, synchronized with presentation, or free of runtime defects.

### FAIL

FAIL means a machine-verifiable gate ran and failed, for example:

- canonical full preflight failed;
- Unreal C++ build failed.

### REVIEW_REQUIRED

REVIEW_REQUIRED is fail-closed uncertainty, for example:

- Git/Python/Unreal is unavailable;
- the working tree is dirty so evidence cannot be tied to one exact commit;
- worker infrastructure encountered an unexpected error.

Uncertainty must never be promoted to PASS.

## Registration

From the repository root on Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\unattended\register_everward_windows_worker.ps1
```

The default task is:

- name: `Everward Unattended Worker`
- trigger: current-user `ONIDLE`
- idle interval: 10 minutes
- elevation: not required by design

The worker can also be run manually:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\unattended\run_everward_windows_worker.ps1
```

## Evidence location

Local evidence is stored at:

```text
%LOCALAPPDATA%\Everward\unattended-worker\
├── latest.json
├── registration.json
├── worker.lock
├── logs\
└── runs\
```

`latest.json` is the latest derived result. `runs\<timestamp>.json` is immutable run evidence unless the user manually deletes local history.

Logs may contain compiler output. The JSON evidence stores only sanitized tails with repository, user-profile, and worker-state paths replaced by placeholders.

## Concurrency and crash recovery

Only one worker run is allowed at a time.

A lock file prevents overlap. If a prior process crashes or the machine reboots and leaves a lock behind, a sufficiently old lock is treated as abandoned and recovered automatically. The default stale-lock threshold is six hours.

## Architectural boundary

This worker does not become a second game runtime and does not manufacture Product Reality.

It must not:

- modify authoritative simulation state to make a test pass;
- edit Unreal assets or user saves;
- bypass failing preflight/build checks;
- treat missing evidence as success;
- use an LLM as test authority;
- claim that a successful compile proves gameplay or visuals.

Simulation still owns truth. Unreal still presents truth.

## Human-in-the-loop retirement plan

The worker is the first step, not the endpoint.

### Slice A — engineering worker (this implementation)

Automate preflight + exact Windows Unreal build.

### Slice B — Unreal runtime smoke worker

Run the deterministic Phase-2 environment automatically and verify startup, pawn/adapter creation, telemetry availability, and clean shutdown.

### Slice C — scripted gameplay driver

Use the same authoritative command paths as the real player to exercise deterministic scenarios such as:

```text
damaged awakening
→ select target
→ scan
→ José approach
→ deploy/select manipulator
→ mine
→ verify storage/material increase
→ Fix_It repair
→ verify material/energy consumption
→ verify subsystem integrity increase
```

The driver records telemetry and assertions instead of asking a human to repeat the sequence.

### Slice D — visual/performance regression

Capture deterministic screenshots/performance checkpoints on the real GPU and flag large regressions such as missing HUD, wireframe mode, misplaced probe/tools, severe clipping, startup failure, or unacceptable memory/performance behavior.

### Slice E — milestone-only human playtest

Once the automated chain is green and a coherent gameplay slice exists, ask the human questions machines cannot answer reliably:

- Is being the probe satisfying?
- Does scanning create curiosity?
- Does mining feel physical and understandable?
- Does Fix_It's reasoning make sense?
- Is the first worker/repair/replication payoff exciting?
- Do you want to keep playing?

The human should no longer be the regression detector.

## Ollama

A local Ollama model may later summarize or cluster sanitized derived failures, but it is advisory only. PASS/FAIL remains determined by deterministic checks and captured evidence.
