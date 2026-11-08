# Phase 2: isolated manipulator IK feasibility pilot

**Status:** experimental C++ only; NOT player-visible or approved for runtime control. Source review/CI and Unreal Product Reality are separate acceptance gates.

## Authorization and architecture

Owner authorized 2026-11-08: inspect OpenIK as an MIT-licensed technical reference, compare constrained CCD/FABRIK with Unreal Engine 5.8, implement a reversible mining-arm IK pilot where feasible, preserve authoritative simulation, manual control, tool-tip contact, collision and owner-control gates, and automate tests. No paid tools, no unrelated changes, no unverified merge.

The authoritative Everward sources remain:
- src/simulation/include/everward/simulation/manipulator.hpp: two Gen-1 arms; shoulder [-90,90], elbow [0,150], wrist [-180,180] degrees; 45 degrees/sec commanded slew; injectable collision guard.
- manipulator_hull_contact.hpp and manipulator_tool_contact.hpp: 1.80 m and 1.50 m segments, 0.79 m wrist-to-contact offset, fold/roll geometry, canonical tip contact and collision samples.
- PHASE2_MANIPULATOR_CONTROL_DISCOVERABILITY.md: joint-by-joint input is an engineering interface; future end-effector control is an intended improvement.
- PROCEDURAL_MECHANISM_STUDIO.md: the separate 3D bridge is NOT equivalent to verified Unreal gameplay.

## Source-backed options comparison — not a hardware benchmark

| Candidate | Useful property | Limitation | Decision |
|---|---|---|---|
| OpenIK constrained CCD (Unity 6) | Joint-by-joint end-to-root position solving, joint clamps | Unity C# / scene transform model, not directly portable to authoritative Everward C++ | Conceptual reference only; no copied code |
| OpenIK constrained FABRIK (Unity 6) | Length-preserving forward/backward passes, singularity handling | Mapping back to signed fold/roll hinges and collision-safe poses requires extra validation | Research candidate for a later controlled numeric benchmark |
| Unreal 5.8 IK Rig / Control Rig | Native IK solvers, goals, animation tools | Visual skeletal state must not determine gameplay contact/physics; needs runtime evidence | Future isolated UE comparison |
| Everward experimental constrained CCD | Uses canonical C++ tool-tip FK, limits, opt-in guard, fixed passes | Position-only, potential local minima, no swept-collision proof | First **isolated implementation**, not wired into game |

Research links:
- OpenIK v0.9.2: https://github.com/WouterApts/openik-unity/tree/v0.9.2
- OpenIK solver documentation: https://github.com/WouterApts/openik-unity/blob/main/Documentation~/core-reference.md
- OpenIK MIT license: https://github.com/WouterApts/openik-unity/blob/main/LICENSE.md
- UE 5.8 IK solvers: https://dev.epicgames.com/documentation/unreal-engine/ik-rig-solvers-in-unreal-engine

## Implemented pilot

Header: src/simulation/include/everward/simulation/manipulator_ik_pilot.hpp

The caller supplies a **fully deployed** arm, probe-local XYZ target (metres), an already-valid starting pose, and preferably the authoritative combined hull/environment collision guard.

1. Reject nonfinite inputs, out-of-range starting angles, and disallowed initial poses.
2. Reject targets outside the 1.80 + 1.50 + 0.79 m reach bound. Passing this necessary test does not imply the target is feasible.
3. Walk wrist to elbow to shoulder; use the exact simulation kinematics to calculate hinge-axis projection and damped CCD adjustments with joint clamps.
4. Accept a candidate only if error decreases and the supplied collision guard accepts that pose.
5. Bound work to 64 fixed iterations. Return success only at <= 0.025 m tool-tip error; otherwise preserve **original** angles, never passing an unsolved pose to the rig.

The 25-mm positional threshold and 64 passes are configured **pilot parameters**, not empirically justified production tolerances.

The function only returns proposed angles. It never commands ManipulatorRig, modifies saves, changes Unreal, bypasses the existing manual controls or rewrites mining contact truth. No third-party code was copied and no Unity package was installed.

## Automated tests

Test source: src/simulation/tests/manipulator_ik_pilot_tests.cpp, registered in CTest as everward_manipulator_ik_pilot_tests.

Tests cover target-at-current-tip, mirrored port/starboard target generated using canonical forward kinematics, deterministic repeats, joint limits, unreachable targets, NaN/invalid seed rejection, guard-rejected changes, and nonmutation of ManipulatorRig/slew rules.

**Evidence state at authoring:** source implemented; CI and Unreal exact-build verification **pending**. CMake registration alone does not prove the suite passes. Solver speed, RAM, game impact and native Unreal equivalence are **unmeasured**. Do not claim visual/gameplay acceptance based on these tests.

## Product Reality/promotion gates

- Compile and run tests at exact PR SHA; diagnose and repair failures before promotion.
- Add test coverage for authoritative combined hull/environment guards, near-singular poses, collision across **intermediate** slew steps and moving targets; final-pose collision checks alone are insufficient.
- Require the actual mining-tip/asteroid contact calculation to remain unchanged, and preserve tool attachment, manual override and power gating.
- Compare Unreal 5.8 native IK in a disposable prototype scene, without letting animation become simulation truth.
- Run approved unattended Windows UE/UBT and low-spec frame time / RAM / tool contact verification. Distinguish source-implemented, Unreal-verified and player-accepted.
- Keep owner-control semantics and existing higher-priority opening-loop work; no automatic merge while materially unverified.

Reversion: remove this experimental header, its CTest test and the three CMake registration lines. No runtime behavior changes.

## License

OpenIK is MIT-licensed, copyright (c) 2026 Wouter Apts. This pilot is independently authored as an evaluation, not a port of OpenIK source. Any later copied source requires the upstream copyright/license notice and dependency review. The separate Unity Asset Store skybox product is not an Unreal code or asset dependency.
