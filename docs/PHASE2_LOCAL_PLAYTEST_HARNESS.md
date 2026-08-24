# Phase 2 Local Playtest Harness

This helper reduces setup friction for the first local Unreal Engine 5.8 integration run. It does **not** change gameplay or replace the structured playtest protocol in `PHASE2_FIRST_RUN_PLAYTEST.md`.

## Run from the repository root on Windows

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_phase2_first_playtest.ps1
```

The harness will:

1. locate the repository and `unreal/Everward.uproject`;
2. locate Unreal Engine 5.8;
3. capture the exact Git commit being tested;
4. create a timestamped observation under `playtests/phase2/observations/`;
5. capture CPU/GPU names when Windows exposes them through CIM;
6. build `EverwardEditor Win64 Development` with Unreal Engine 5.8 `Build.bat`;
7. automatically mark `unreal_cpp_build` pass/fail in the observation;
8. record a build failure as a blocker and stop rather than launching a broken editor target;
9. launch `UnrealEditor.exe` with the log window after a successful build;
10. print the observation path and validation command.

After Unreal opens, continue with `docs/PHASE2_FIRST_RUN_PLAYTEST.md`.

## Unreal Engine discovery

The harness tries, in order:

- `-UnrealRoot` supplied on the command line;
- `UE58_ROOT`, `UE_5_8_ROOT`, or `UE_5_8` environment variables;
- common Epic Games Unreal Engine 5.8 registry entries;
- `C:\Program Files\Epic Games\UE_5.8`;
- `C:\Epic Games\UE_5.8`.

If automatic discovery fails:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_phase2_first_playtest.ps1 -UnrealRoot "D:\Epic Games\UE_5.8"
```

Use the actual UE 5.8 installation root on the machine.

## Optional switches

Prepare evidence and launch without rebuilding:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_phase2_first_playtest.ps1 -SkipBuild
```

Prepare evidence and build without opening Unreal:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_phase2_first_playtest.ps1 -NoLaunch
```

For the canonical first-run evidence, do **not** use `-SkipBuild`; the Unreal C++ build is a required objective check.

## Evidence behavior

The generated observation begins with every required check as `not_tested`. If the harness completes the UBT build successfully, it changes:

```text
unreal_cpp_build -> pass
overall_result   -> partial
```

If UBT fails, it records:

```text
unreal_cpp_build -> fail
overall_result   -> fail
```

and adds the build failure to `blockers`.

The remaining checks are intentionally left for the actual playtest rather than guessed from process launch.

Validate the completed observation with the command printed by the harness, or manually:

```powershell
python .\tools\validate_phase2_first_run_observation.py .\playtests\phase2\observations\<observation-file>.json
```

## Design boundary

This harness exists because the current Phase 2 gate is evidence. It must not become a second gameplay runtime, modify authoritative simulation state, alter editor assets to make a test pass, or hide Unreal warnings/errors.

A failed local build or failed objective check is useful evidence and takes priority over adding new gameplay slices.
