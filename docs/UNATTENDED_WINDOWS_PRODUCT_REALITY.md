# Everward Unattended Windows Product Reality

Issue: #240

## Product requirement

The Windows laptop remains part of Everward's Product Reality loop. The user should not be the test harness.

> **Zero humans for facts a computer can measure.**

A human remains valuable for actual game feel, visual/art direction, control feel, readability preference, and the high-level question "do I want to keep playing?". Repository, build, deterministic simulation, exact-commit identity, and Unreal C++ compile facts belong to automation.

This worker is intentionally narrower than a complete automated Unreal playtest. It retires the first large block of routine laptop work without pretending that a successful build proves the game feels good.

## One-time activation

First-time activation is deliberately explicit. An ordinary development clone or normal playtest may **not** silently authorize destructive unattended synchronization.

Use the dedicated setup launcher:

```text
tools\Setup_Everward_Unattended_Testing.bat
```

The launcher operates only on the disposable playtest checkout at:

```text
%USERPROFILE%\Documents\Everward Playtest\Project-Everward
```

It finds the latest successful main-branch Foundation commit, prepares that dedicated checkout, and calls `tools/register_unattended_product_reality.ps1` with `-ConfirmDedicatedCheckout`. That explicit switch is the only normal first-time path that creates the `.git/everward-unattended-worker` safety sentinel.

After authorization, the existing Phase-2 harness may non-blockingly **refresh** the scheduled task when it sees the sentinel. It cannot create the sentinel itself, so running a normal playtest from another clone cannot convert that clone into a destructive worker checkout.

Registration:

- revalidates the expected `jweter/Project-Everward` origin;
- creates or refreshes the current-user Windows Task Scheduler task `Everward Unattended Product Reality`;
- runs after approximately 10 minutes of Windows idle time;
- uses `LIMITED` privilege and requests no elevation;
- can be disabled by setting `EVERWARD_DISABLE_UNATTENDED_WORKER=1`, after which the registration helper removes any existing task.

The one-time setup also checks GitHub CLI. If it is available and authenticated, the worker updates issue #243 with a compact sanitized latest status. Detailed logs and paths remain local.

## What the worker does

Scheduled execution calls the reporting wrapper, which runs:

```text
python tools/everward_unattended_worker.py --repo-root <dedicated-playtest-checkout>
```

The worker then:

1. proves the checkout is the explicitly authorized dedicated test checkout by requiring the `.git/everward-unattended-worker` sentinel;
2. proves `origin` is `jweter/Project-Everward` before allowing reset/clean operations;
3. refuses to mutate the checkout while a manual playtest/build lock, `UnrealEditor.exe`, or UnrealBuildTool process is active;
4. queries the GitHub Actions `foundation.yml` workflow once for the latest successful **main-branch push** commit;
5. preserves existing Phase-2 observation files outside the checkout before destructive cleanup;
6. fetches origin and checks out that exact previously discovered commit detached;
7. runs the repository-standard `python tools/quality_preflight.py --full` gate;
8. only if full preflight passes, locates Unreal Engine 5.8 using the same explicit/env/registry/common-path model as the manual harness;
9. builds `EverwardEditor Win64 Development` through UnrealBuildTool;
10. only if the build passes, runs the headless Unreal map/load smoke (`tools/run_unreal_headless_smoke.ps1`);
11. only if the headless smoke passes, runs the bounded low-spec startup/memory probe described below;
12. writes exact-commit PASS / FAIL / REVIEW_REQUIRED evidence locally;
13. caches a successful exact commit so an unchanged green build is not rebuilt on every idle episode — the cache requires both the headless-smoke and low-spec-startup markers for the same commit;
14. best-effort updates GitHub issue #243 with the latest sanitized status when GitHub CLI is authenticated.

### Low-spec startup/memory evidence

`tools/run_unattended_low_spec_startup.ps1` reuses the opt-in Low-Spec Development launch contract of `tools/run_phase2_first_playtest.ps1 -LowSpec -MeasureMemory`: the same windowed 1280×720 editor launch, the same `-ExecCmds` console-variable list, and the same `UnrealEditor` process `WorkingSet64` memory source. A regression test (`tools/test_unattended_low_spec_startup.py`) fails if the two argument lists drift apart. The worker adds only `-Unattended` (suppress modal dialogs) and a worker-owned `-log=` path. Production/default settings, `LowSpecPlay.ini`, saves, and simulation mechanics are not touched, and no Phase-2 observation file is written.

The probe is bounded: it waits at most 900 seconds for the engine initialization log marker (`Engine is initialized.` or `(Engine Initialization) Total time:`), then samples process memory for a further 60 seconds, then always terminates the editor process tree it launched. It writes a path-free JSON result containing only the exact commit, marker/exit/cleanup booleans and enums, elapsed seconds, engine-reported init seconds when present, peak working set MiB, and sample counts.

The worker interprets that result fail-closed as check `unreal_low_spec_startup`:

- `PASS` — the exact tested commit reached the engine initialization marker within the bound, stayed alive for the full post-startup sample window, produced positive working-set samples, and was confirmed stopped;
- `FAIL` — startup timed out, the editor exited before or during the window, or the probe itself exceeded its outer bound (allow-listed `failure_category`);
- `REVIEW_REQUIRED` — the result was missing/malformed, the commit identity did not match, the editor was not launched or could not be confirmed stopped, the window was incomplete, or memory telemetry was unavailable.

The check records `build_identity` (exact commit, `EverwardEditor Win64 Development`, and the Unreal Engine version/changelist from `Engine/Build/Build.version`), `product_reality_claimed: false`, and an explicit `not_evidence_for` list. A low-spec `PASS` is **not** evidence of visual quality, control feel, HUD readability, frame time/playability, or gameplay-loop quality; those remain human-only debt. Failure tails and probe errors are redacted (dedicated checkout, Unreal root, user profile, and Windows user name, case-insensitively), and the published issue body carries only the status, an allow-listed failure category, startup seconds, and peak working-set MiB.

The scheduled worker always exits zero to Task Scheduler. Test truth is in the evidence JSON, not in a retry storm. GitHub publication is presentation only and cannot change PASS/FAIL truth.

## Local evidence

Default state lives outside the repository:

```text
%LOCALAPPDATA%\Everward\unattended-worker\
  latest.json
  last_passed_headless_smoke_commit.txt
  last_passed_low_spec_startup_commit.txt
  history\<timestamp>.json
  logs\<commit>\full-preflight.log
  logs\<commit>\unreal-build.log
  logs\<commit>\unreal-headless-smoke.log
  logs\<commit>\unreal-low-spec-startup.log
  logs\<commit>\unreal-low-spec-startup.json
  preserved-playtests\<timestamp>\phase2-observations\...
```

Nothing in this directory is committed automatically.

The worker records:

- exact target/tested commit;
- dedicated-checkout/origin safety status;
- canonical full-preflight status, exit code, duration, and local log;
- Unreal Engine 5.8 discovery status;
- UBT build status, exit code, duration, and local log;
- headless Unreal smoke status;
- low-spec startup/memory status with exact build identity, startup seconds, and editor peak working set;
- a redacted failure tail that replaces the dedicated checkout, user profile, and Unreal installation roots;
- the explicit human-only debt that remains.

Top-level results:

- `PASS` — every deterministic lane available in this slice passed;
- `FAIL` — an executed deterministic assertion/build failed;
- `REVIEW_REQUIRED` — required evidence/tooling was missing or the worker safely deferred;
- `BUSY` — another worker instance owns the local lock.

The lock stores the owning PID. A dead/stale owner is reclaimed automatically so a crash or reboot does not permanently disable unattended testing.

## Safety boundaries

The worker may reset/clean **only** a checkout that carries the explicitly created dedicated sentinel and expected GitHub origin. It does not run against an arbitrary development checkout.

Before reset/clean, current Phase-2 observation evidence is copied to local worker preservation storage. Manual Unreal builds/playtests and unattended checkout mutation are serialized so the scheduled worker cannot switch source underneath UnrealBuildTool.

The worker does not:

- launch PIE or manipulate the Unreal UI (the low-spec probe only launches the editor, waits, samples process memory, and terminates it);
- modify authoritative simulation state to make a test pass;
- modify player saves;
- commit, push, or merge source code;
- upload detailed local logs;
- declare rendering, controls, art, frame-time, or gameplay feel correct from a successful compile;
- replace Everward's existing deterministic simulation CI;
- turn missing evidence into PASS.

The architecture boundary remains unchanged: **simulation owns truth; Unreal presents truth.**

## What this retires from routine human testing

Once registered, the user should not be asked to repeatedly establish:

- whether the current exact green commit can be checked out on the laptop;
- whether full repository preflight passes on that machine;
- whether Unreal Engine 5.8 can be found;
- whether `EverwardEditor Win64 Development` compiles;
- what exact commit produced a build failure;
- whether the same already-passed commit needs another identical build.

A deterministic failure discovered by a human should gain automated regression or worker coverage before the same fact is requested again.

## Human-only Product Reality that remains

Human testing remains valid for questions such as:

- Does inhabiting the Prime probe feel convincing?
- Are movement and manipulator controls understandable and satisfying?
- Does collision/contact look and feel physically believable?
- Is the HUD readable without developer knowledge?
- Do graphics and audio communicate the intended machine sensorium?
- Is Low/Minimum actually pleasant enough to play on the target laptop?
- After the opening loop, does the player want to continue?

Those are product judgments, not substitutes for machine-verifiable engineering checks.

Human playtests should become milestone events rather than routine regression labor.

## Next automation slices

The natural follow-on work for #240 is:

1. add deterministic Unreal runtime/automation hooks for map load and adapter startup so the worker can verify more than compilation without human interaction;
2. script the opening deterministic gameplay chain: damaged awakening → target selection → scan → José approach → manipulator/mining → material gain → Fix_It repair;
3. assert telemetry and state transitions from that chain automatically;
4. add deterministic screenshots and asset/reference checks for gross visual regressions such as wireframe mode, missing HUD, missing probe/tools, or severe camera clipping;
5. add instrumented frame-time capture for the low-spec Product Reality target (bounded startup and editor working-set capture now exist; frame time does not);
6. feed deterministic worker evidence into the existing read-only mobile evidence surface;
7. optionally use local Ollama to classify sanitized derived failure evidence, while keeping Python/tests/build tools as authority;
8. retire each human checklist item as soon as an independent machine oracle exists.
