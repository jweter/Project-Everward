# Phase 2 Local Unreal Build Failure — 2026-09-01

## Product Reality evidence

The latest-passed-build launcher selected commit `6c34acd5374834a265059a6b4cd2db523d0691af` (merged PR #152) and correctly reached the local Unreal Engine 5.8 `EverwardEditor Win64 Development` build step. UnrealBuildTool compiled far enough to enter the link stage, then failed linking `UnrealEditor-Everward.dll` with 12 unresolved JSON symbols from `PlaytestRecorderActor.cpp`, including `FJsonValue::AsNumber`, `FJsonValue::AsString`, `FJsonValueObject`, `FJsonObjectSharedStringStorage`, and `LogJson`.

The production `Everward` module used JSON serialization in `PlaytestRecorderActor.cpp` but `unreal/Source/Everward/Everward.Build.cs` declared only `Core`, `CoreUObject`, `Engine`, and `InputCore`. The separate rendering-benchmark Unreal module already declares the relevant JSON dependencies, so the production module was missing its link-time dependency rather than failing because of the selected-target material work in PR #152.

## Root cause

`PlaytestRecorderActor.cpp` includes and uses Unreal's JSON API, but the production Unreal module did not link the `Json` module. Header availability allowed compilation, while the missing module dependency surfaced at DLL link time as `LNK2019`/`LNK1120` unresolved externals.

## Fix

Add `Json` to `PrivateDependencyModuleNames` in `Everward.Build.cs`. JSON types are only used from the `.cpp`, so a private module dependency is sufficient and keeps the public module surface minimal.

A source-contract regression test, `tools/test_unreal_playtest_recorder_dependencies.py`, verifies that JSON serializer/writer usage remains paired with a declared `Json` module dependency.

## Acceptance

The next laptop Product Reality run must:

1. select the new CI-passed commit containing this fix;
2. complete `EverwardEditor Win64 Development` without the JSON linker errors;
3. launch Unreal Engine automatically;
4. continue the Phase 2 Product Reality checks, including PR #152's target-selection highlight.

The Visual Studio 14.51 preview-toolchain warning, XGE license fallback, and antivirus advisory in the failed build are not the primary failure: UnrealBuildTool reached the linker and reported concrete missing JSON symbols. Those warnings should remain monitored but are not treated as this incident's root cause.
