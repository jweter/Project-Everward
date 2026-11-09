# G03: First native Unreal gameplay regressions (free, milestone-aware)

**Issue:** [#378](https://github.com/jweter/Project-Everward/issues/378), G03; supports
G05 survey persistence and the controlled-rejection portion of G01/G02.

**Owner rule reaffirmed November 8, 2026:** Do **not** initiate a paid Aura
playtest after each development slice or each G gate. Aura is owner-run only,
reserved for **major integrated playable milestones** at the owner's choice.
Use free local simulation tests, real native Unreal Automation tests, and
approved unattended Windows evidence during normal engineering.

## Exact tests registered by this source slice

- `Everward.G03.Native.ScanSurveyPersistsAfterCancel` creates a game-type
  Unreal test world, spawns the real Everward probe pawn, starts real actor
  BeginPlay, powers Sensors, starts a scan through the gameplay adapter, ticks
  the **Unreal world** and checks actual ScanCompleted and persistent surveyed
  state. A subsequent CommandCancelScan must produce a cancellation event
  without erasing the previous successful survey.
- `Everward.G03.Native.MiningRejectsWithoutSurveyAndTool` starts a fresh
  native game-world actor and issues an actual mining command. Without survey
  and a deployed/attached tool it must reject. Deposit, extracted kilograms,
  authoritative storage and material inventory must not change.

**These are not synthetic JSON tests or direct C++ simulation-core tests.**
They exercise the Unreal module and actual actor/component lifecycle. The
source also includes cheap Python contract tests; those verify source
registration and runner safeguards only, **not Unreal execution**.

## Run on an approved idle Windows worker after exact-head UnrealBuildTool compile

The opt-in runner **first builds EverwardEditor from the same clean Git SHA**
using UnrealBuildTool with only one parallel compile action; it aborts on
failure or memory pressure. A previous binary or green portable Foundation
CI does not count as compilation evidence for this checkout.

```powershell
powershell -NoProfile -File tools\run_unreal_native_g03.ps1
```

The opt-in runner checks repository provenance, clean HEAD, Unreal 5.8
availability, absence of competing Unreal/editor/build processes, a
10-GiB initial free-RAM gate and a continuous 7-GiB memory floor during
compilation and test execution. It uses `UnrealEditor-Cmd.exe`, not Aura; stores
private logs and the real Unreal Automation JSON report under
`%LOCALAPPDATA%\Everward\native-g03\<sha>\<timestamp>\`; requires two
named tests and a passing aggregate report; rejects any missing evidence.
The engine's existing owner control and other worker leases take priority.
This script does not register/schedule a new Windows task or modify the
running Iron God release.

## Evidence, limitations and next steps

At PR creation the C++ tests are **SOURCE_IMPLEMENTED, NATIVE_UNVERIFIED**:
there is no real Unreal 5.8 output, startup proof, PIE input proof or game
capture attached yet. A successful syntax check, Python test, or UBT compile
does **not** upgrade this status. The native runner is not to be invoked on
a busy or memory-constrained laptop; use the existing authorized idle worker
once fully integrated and appropriately resource-gated.

G03 remains **OPEN** until the exact-head native run is repeated and
preserved with logs and a report. This slice **does not prove** an accepted
G01 mining cycle, P-key approach, true tool contact, material collection,
Fix_It repair, full keyboard/controller interaction, or player-visible HUD.
Those require subsequent real gameplay/native assertions. No paid Aura
playtest is needed for this routine coverage.
