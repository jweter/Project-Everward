# Everward Project Status

This file is the durable operational continuation record for human and scheduled development. It answers: **where are we now, what is verified, what remains open, and what should happen next?** Detailed historical implementation narratives belong in Git history, PRs, playtest evidence, and `ERROR_RESOLUTION_LEDGER.md`.

## Current phase

**Phase 2 — One Probe: ACTIVE.**

Phase 1 is complete and Unreal Engine 5.8 is the accepted production direction. The first serious local Phase-2 run proved the integrated simulation/Unreal shell works, but the product gate **“simply existing as the probe is compelling”** is not yet met.

Canonical first-run evidence remains:

- `playtests/phase2/observations/phase2-first-run-20260824-225821.json`
- `docs/PHASE2_FIRST_RUN_FINDINGS_2026-08-24.md`
- tested commit: `7f9ea88b8e7857f44f80b2f2327fde758dd2ca1a`

The repository has advanced substantially beyond that first-run build through the explicit parallel-safe lane. Those later mechanics are **implemented but do not count as locally accepted Product Reality until the exact Unreal test passes.**

Current `main`: `fb0e69f152bc606c3ba54cc61e1cc17cc2f24c89`, after the compound contact-envelope solver wiring (#131). The evolving-sensorium-audio foundation (#122) and the Prime Generation-1 body blockout (#123, Slice 5) are folded in below alongside the manipulator arm work.

The documentation debt previously flagged here for #125–#128 (Prime functional material pass, the Prime A tube-body/visible-arm-geometry replacement, and the fore-aft cylinder axis fix) is resolved by this pass: see the updated Slice 5 and Slice 6 sections below, and the expanded Product Reality sequence, which now also exercises `PHASE2_SIMPLE_PRIME_A_EMBODIMENT_PASS.md`.

## Verified local foundation

The first local Unreal Engine 5.8 run confirmed:

- Unreal C++ build and PIE launch;
- deterministic EV-0001 spawn and generated Phase-2 environment;
- camera orbit/zoom;
- authoritative translation and full stop;
- capability discovery/selection;
- power allocation across Propulsion, Sensors, Computation, and Thermal Control;
- sensor scan completion and cancellation;
- Basic Survival policy install/clear;
- computation execution gate below/above 25 W.

The original subjective baseline remains:

- embodiment: **2/5**
- HUD clarity: **2/5**
- control discoverability: **3/5**
- Generation-1 clunkiness: **4/5**
- movement readability: **3/5**
- automation comprehension: **1/5**
- desire to continue: **1/5**

That baseline is the comparison point for the next serious local build.

## Implemented since the first-run baseline

### Probe orientation and camera-aligned righting

Merged work now provides:

- authoritative yaw/pitch/roll state;
- probe-relative translation;
- asymmetric temporary orientation cues so forward/up/roll are visually legible;
- camera-aligned `R` righting through the existing simulation adapter rather than direct Unreal rotation;
- Space remaining an absolute full stop.

**Status: implemented, Product Reality pending.**

### Subsystem consequences and automation explanation

The systems HUD and runtime now expose more than raw wattage:

- all installed subsystems show live power and `READY` / `LOCKED` / `FAILED` state;
- reason text explains limitations such as minimum-power lockout;
- Sensors require 50 W for active scanning;
- dropping below the minimum aborts an active scan through authoritative mechanics;
- rejected commands produce visible global feedback;
- Basic Survival can visibly report cause/effect such as an automatic Sensors power shed;
- manual and automated changes converge on the same runtime commands.

**Status: implemented, Product Reality pending.**

### Physical contact foundation

Phase-2 contact mechanics are now engine-independent and deterministic:

- EV-0001 has a temporary physical collision envelope separate from decorative art;
- registered physical bodies can block inward motion;
- swept contact prevents simple high-speed tunneling through the test target;
- glancing contact can preserve tangential motion;
- contact records body ID, point, surface normal, relative velocity, inward normal speed, and tick;
- Unreal displays contact but does not own mechanical truth.

**Status: implemented, Product Reality pending.** The local test must prove the probe does not ghost through the target and that contact behavior remains understandable in the actual UE build.

### Compound contact-envelope solver (replaces the single bounding sphere)

The Prime Generation-1 embodiment pass (#127) made the single 8 m bounding
sphere's mismatch with the long, winged Prime silhouette visible: contact
could register in space that visibly did not touch the hull. The compound
contact-envelope foundation and sweep/resolution math landed next (#129,
#130) as engine-independent geometry, and this pass wires that math into
`ProbeRuntime`'s actual swept-contact solver in `software_policy.hpp`:

- the probe's five authoritative local-space collision samples
  (`ProbeCompoundCollisionEnvelope`: nose, central hull, aft, port wing,
  starboard wing), rotated by the probe's current attitude, are now what the
  swept solver tests against registered bodies — not one oversized sphere;
  a nose-first or wing-first approach is now stopped by the corresponding
  sample instead of an invisible boundary around empty space;
- the earliest genuine hit across all five samples is resolved through
  `resolve_compound_contact`, which places the winning sample just outside
  the struck body and derives the resolved probe-root position from it;
- the legacy `collision_envelope_radius_m` field remains only as a coarse
  diagnostic/telemetry number (still read by the existing Unreal HUD
  telemetry) and is no longer consulted by the solver itself;
- deterministic/source-contract coverage: updated `contact_physics_tests`
  and `impact_damage_tests` expectations for the new sample-based contact
  geometry, plus a new source-contract test proving the compound solver is
  actually wired into `ProbeRuntime` rather than merely present alongside it.

**Status: implemented, Product Reality pending.** No Unreal-side files
changed in this pass; the existing 8 m Unreal collision mirror and HUD
telemetry are unchanged, so this is authoritative-simulation-only work
behind the existing adapter boundary. The next local Unreal Product Reality
pass should specifically re-check item 8/9 in the sequence below (approach
the registered physical target nose-first and from the side) to confirm the
narrower, shape-matched contact reads as correct rather than the previously
known oversized-sphere stand-off.

### Impact severity and component integrity

The contact record now feeds a deterministic damage foundation:

- impact severity derives from physically meaningful kinetic-energy inputs rather than arbitrary ship hit points;
- Sensors, Propulsion, Computation, and Thermal Control carry component integrity in `0..1`;
- integrity bands expose offline/critical/degraded/operational/nominal condition;
- cumulative impact damage can impair or disable specific components;
- partial Propulsion integrity reduces local velocity/attitude trim authority;
- partial Sensors integrity increases scan time;
- catastrophic damage routes through existing subsystem-operational consequences;
- damage-aware policy ordering and integrity/operational synchronization were corrected during review before merge;
- Unreal receives impact/integrity telemetry through the simulation adapter.

**Status: implemented, Product Reality pending.** Collision acceptance still precedes claiming damage behavior complete.

### Prime Generation-1 probe body blockout (Slice 5)

The tiny engineering-shell placeholder is replaced with a modular ~15 m Prime Generation-1 blockout: structural spine, computation/core and power/reactor housings, main propulsion, forward sensor hardware, paired radiators and maneuvering pods, a dorsal marker, and paired manipulator shoulder mounts. The authoritative collision envelope and Unreal mirror both moved to an 8 m bounding sphere, and camera orbit/zoom was rescaled for the larger body.

Three follow-on presentation passes then iterated this blockout into the current body:

- **#125** added a functional material/skin pass distinguishing structural alloy, protected core, reactor, propulsion, optics, radiators, maneuvering hardware, sensor mast, and manipulator-joint materials, plus a reusable rock/regolith treatment for the physical scan/contact target;
- **#127** replaced the original disconnected primitive scatter with a single coherent Prime A tube-body silhouette (central tube, two radiator wings, rear propulsion, forward sensor head) and added the visible shoulder/upper-arm/elbow/forearm/wrist/tool-head geometry for both manipulator arms, described under Slice 6 below;
- **#128** fixed a fore-aft axis mistake (UE's `BasicShapes/Cylinder` is long on local Z, so the original yaw rotation left the tube upright; corrected to a 90° pitch) across the central tube, core/reactor sleeves, rear engine, forward sensor, and tool heads.

**Status: implemented, Product Reality pending.** See `PHASE2_SIMPLE_PRIME_A_EMBODIMENT_PASS.md` in addition to the original `PHASE2_PRIME_GEN1_BODY_BLOCKOUT_TEST.md`.

### Evolving machine sensorium and audio progression

`EvolvingSensorium` is a new engine-independent foundation module (parallel to `AdjacentGenerationEvolution`) modeling audio/perception as something a lineage earns rather than a stock soundtrack: essential accessibility cues are never progression-gated, while internal-machine perception, contact/vibration sensing, electromagnetic and scientific sonification, adaptive ambience, generative music, vocal synthesis, and songwriting are reachable adjacent upgrades keyed off installed traits and computation/science maturity.

**Status: foundation architecture; not yet wired to Unreal audio presentation.**

### Manipulator arm mechanics, HUD, and visible geometry (Slice 6)

The first sub-slice of **Slice 6 — articulated manipulator arms** lands on the two shoulder mounts Slice 5 already exposes:

- new engine-independent `ManipulatorRig` (Port/Starboard arms, each with shoulder/elbow/wrist joints);
- deploy/stow is gradual and reversible mid-transition rather than an instant flag flip, and fires its lifecycle event exactly once per transition;
- joint targets are only accepted on a fully deployed arm, are clamped (not rejected) to a constrained range, and slew toward the target at a bounded rate;
- a tool interface can only attach to a deployed arm, and an arm cannot stow with a tool still attached;
- `UProbeSimulationAdapter` exposes deploy/stow/joint/tool commands and per-arm telemetry, advanced on the same fixed-step cadence as the rest of the probe;
- minimal input (`1`/`2` deploy-stow toggle per arm, `3` tool attach/detach toggle) and a compact HUD status line per arm.

**Status: implemented, Product Reality pending.** See `PHASE2_MANIPULATOR_ARM_FOUNDATION_TEST.md`.

A second sub-slice adds the joint articulation input and dedicated manipulator HUD page that foundation deferred, over the same already-authoritative joint commands:

- `UProbeSimulationAdapter`'s per-arm telemetry now also reports each joint's commanded target, not only its current (slewing) angle, so input can nudge from the last intended pose;
- a dedicated `M`-toggled manipulator HUD page shows both arms, the selected arm/joint, and current -> commanded degrees per joint, with `DEPLOY ARM TO COMMAND JOINTS` shown in place of joint rows for a stowed arm;
- `N` cycles the arm joint input targets, `4`/`5`/`6` select Shoulder/Elbow/Wrist, `,`/`.` nudge the selected joint's commanded target;
- no engine-independent simulation behavior changed; clamping and slew-rate limiting remain entirely inside `manipulator.hpp`.

**Status: implemented, Product Reality pending.** See `PHASE2_MANIPULATOR_JOINT_ARTICULATION_TEST.md` for the local test script.

A third sub-slice, landed alongside the Prime A embodiment pass (#127) rather than as a dedicated manipulator PR, adds the visible geometry those mechanics were missing:

- a `shoulder pivot -> upper arm -> elbow pivot -> forearm -> wrist pivot -> tool head` mesh hierarchy for both arms, attached at the shoulder mounts Slice 5 exposes;
- `AEverwardProbePawn::UpdateManipulatorVisuals()` drives that hierarchy every tick purely from `UProbeSimulationAdapter::GetManipulatorArmStates()` (deployment fraction and current, not commanded, joint angles) — no visual component authors manipulator mechanics;
- deploy/stow reads as a fold/unfold of the shoulder pivot rather than an instant show/hide, and the tool head visibly changes scale on attach/detach.

**Status: implemented, Product Reality pending.** See `PHASE2_SIMPLE_PRIME_A_EMBODIMENT_PASS.md` items 4–7.

All three Slice 6 sub-slices (foundation mechanics, joint-articulation input/HUD, visible geometry) are now implemented, and Product Reality evidence is outstanding for all three. Slice 6 also has one stated requirement still unimplemented, independent of that evidence: `PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 6 player-visible result list includes "collision does not allow impossible penetration through the probe body," but every arm mesh is built through `ConfigureMesh`, which sets `ECollisionEnabled::NoCollision` — there is no arm/body or arm/environment collision yet, and none of the three test scripts above check for it. Slice 6 is not complete until that collision requirement is implemented in addition to the local Unreal Product Reality pass.

### Canonical damaged awakening and Self Repair direction

The canonical opening is now durable project direction:

`damaged planetary/moon awakening -> minimal power -> reachable mining/manipulation -> staged self-repair -> capabilities return -> all required starter systems reach 100% -> first departure`

Self Repair follows the rule **restore capability before restoring perfection**:

1. preserve immediate existence;
2. restore important offline systems to minimum useful operation;
3. bring required systems online before polishing one system to 100%;
4. reassess integrity, strategic value, mission need, cost, and unlock value;
5. repair in useful stages toward 100%;
6. reassess after every repair.

The player retains agency over the recommendation and material/energy/time cost. Current component-integrity work is intentionally shaped so this later recovery loop does not require a replacement damage model.

### Open-ended successor evolution

Adjacent-generation evolution now explicitly preserves both:

- **improve an existing system**;
- **add a new capability**.

Specialization has no arbitrary maturity cap. Costs/tradeoffs grow instead of a hard technology ceiling, and the player sees only the reachable frontier rather than a spoiler-heavy complete technology tree. This is foundation architecture; it does not advance the current Phase-2 embodiment gate by itself.

### Repository security

The repository now has the pinned portfolio TruffleHog introduced-change gate with read-only history access, `--no-verification`, suppressed findings, and an ephemeral detection self-test. This is security-only and does not alter simulation/gameplay behavior.

## Current authoritative foundation

Everward continues to preserve:

- engine-independent C++20 simulation under `src/simulation/`;
- deterministic fixed-step simulation time;
- canonical EV-0001 identity, generation, mass, motion, power, thermal, storage, capability, component-integrity, and subsystem state;
- scan lifecycle and power-budget validation;
- energy/thermal lockouts and recovery hooks;
- Generation-1 software-policy evaluation;
- manual and automated actions converging on authoritative mechanics;
- capability-driven Unreal HUD/control shell and `UProbeSimulationAdapter` boundary;
- deterministic physical-body/contact records;
- component-specific impact/damage foundation;
- Prime A tube-body silhouette with functional materials and a rescaled collision envelope (Slice 5);
- adjacent-generation evolution foundation;
- evolving machine sensorium/audio progression foundation;
- manipulator arm deploy/stow/joint/tool foundation, joint articulation input and a dedicated manipulator HUD page, and visible shoulder/arm/tool-head geometry (Slice 6; implemented, Product Reality pending);
- canonical Prime Probe A / Scientific Explorer reference package with provenance validation;
- explicit versioned save architecture rather than blind Unreal object serialization.

## Exact next local UE 5.8 Product Reality pass

Use the exact passed build and validate the accumulated Phase-2 chain in order:

1. confirm EV-0001 reads as one continuous tube-bodied Prime A probe from front, side, rear, and top, with both radiator wings, rear propulsion, and forward sensor obvious at a glance;
2. confirm both manipulator arms' shoulder/upper-arm/elbow/forearm/wrist/tool-head geometry is visible under/alongside the body even while stowed, not represented by HUD state alone;
3. yaw, pitch, and roll both directions; confirm visible probe orientation changes correctly;
4. put EV-0001 into an awkward attitude, orbit the camera, press `R`, and verify the stepped camera-aligned righting aid is understandable and still simulation-authoritative;
5. translate after attitude changes and verify motion is probe-relative; confirm Space still performs an absolute full stop;
6. verify all subsystem rows show live watts, state, and reason text;
7. start a scan with Sensors at 50 W, reduce below minimum, and verify authoritative abort + visible lock/rejection reason;
8. restore Sensors and verify scanning returns;
9. install Basic Survival and verify a deterministic visible automation cause/effect event without fabricated discoveries;
10. approach the registered physical target at low speed and verify solid contact rather than ghosting through it, and that the narrower five-sample compound envelope reads as shape-matched rather than the previously known oversized-sphere stand-off;
11. repeat with a glancing approach and verify tangential motion is preserved plausibly;
12. create a damaging impact and verify impact severity, affected component, integrity change, and resulting subsystem consequences agree;
13. verify no stale policy action executes after Computation is destroyed and no operational/integrity state contradiction appears;
14. deploy an arm, open the manipulator page (`M`), and verify each joint's commanded target can be selected (`4`/`5`/`6`) and nudged (`,`/`.`) independently, with the current angle visibly slewing toward the commanded target rather than snapping, the visible upper-arm/forearm geometry moving in sync with that readout rather than only the HUD numbers changing, and joint commands rejected while the arm is not deployed;
15. record visual overlap, clipping, unreadable telemetry, implausible contact, damage mismatch, or control confusion as Product Reality evidence;
16. repeat the embodiment/HUD/control/clunkiness/movement/automation/desire-to-continue ratings against the first-run baseline.

A failure in orientation/control or physical contact outranks later roadmap work. A damage-layer failure blocks Slice 4 completion. Portable CI is not a substitute for this test.

## Authorized next development

Priority order:

1. complete the accumulated local Phase-2 Product Reality pass above;
2. repair any failed orientation, subsystem, contact, or damage behavior before building on it;
3. **Slice 6 — articulated manipulator arms**: mechanics, joint-articulation HUD, and visible geometry are all now implemented, but arm/body/environment collision (a stated Slice 6 requirement) is not — every arm mesh currently has `ECollisionEnabled::NoCollision` — so Slice 6 needs that collision work in addition to recording the local Product Reality pass above across its three test scripts;
4. then move into object selection and physical interaction;
5. keep later planetary/resource/fabrication/repair slices aligned with the canonical damaged-awakening sequence.

While local Product Reality is unavailable, only work that satisfies the explicit parallel-safe lane in `PHASE2_VERTICAL_SLICE_PLAN.md` may merge. Do not build new mechanics that assume unverified contact/damage behavior is correct.

## Product priorities after acceptance

### Prime Probe embodiment

The temporary orientation skin remains engineering scaffolding. The next serious embodiment test should use a recognizable Generation-1 Prime Probe A blockout with identifiable structural spine, core/computation region, power hardware, propulsion, sensors, thermal radiators, and manipulator mounts.

### Manipulators

At least two articulated arms should begin with constrained shoulder/elbow/wrist/tool-interface motion and a clean authoritative command/state path suitable for later servicing, instruments, grabbing, mining, and construction.

### Physical interaction

The next embodied gameplay loop remains:

`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`

### Recovery architecture

Damage, mining, processing, fabrication, and Self Repair must converge on identifiable components and real materials/energy/time costs. Avoid a monolithic health bar or invisible resource teleportation.

## Open architecture decisions

### Final Generation-1 flight model

Current command-driven attitude and translation are deliberate Phase-2 mechanics, not a claim that final thruster/rigid-body flight is solved. Keep truth deterministic and simulation-owned while eliminating world-axis rail feel.

### Final player programming interface

Basic Survival remains an architectural proof. Richer scripts, priorities, diagnostics, and compute-scaled automation are later work. Manual and automated actions must continue to converge on shared authoritative commands.

### Final repair semantics

Current component integrity is the authoritative bridge into staged Self Repair, but exact repair recipes, minimum useful thresholds, fabrication requirements, and field-repair rates remain later industrial/recovery design work.

## Phase-2 production rules

- Simulation owns mechanical truth.
- Unreal consumes authoritative state and submits commands through the adapter boundary.
- `src/simulation/` remains buildable/testable without Unreal dependencies.
- Manual and automated controls share authoritative mechanics.
- Canonical simulation units stay engine-independent; Unreal conversion happens at the presentation boundary.
- Deterministic headless execution remains required.
- Save data remains an explicit versioned schema.
- Later descendants expose capabilities from installed hardware/software, not a universal ability list.
- Better computation may enable richer automation/design capability but cannot bypass physical hardware, materials, manufacturing, or known science.

## Visual product constraint

Everward remains a cinematic, immersive, high-fidelity 3D scientific-realism project. It must not drift toward a primarily 2D, abstract-map, low-poly, deliberately quirky, or visually lightweight interpretation merely because that is easier to implement.

Temporary primitives are acceptable only as engineering scaffolding. Each serious embodiment test should move closer to the canonical Prime Probe A target.

## Automation operating state

Scheduled development follows `AGENT_DEVELOPMENT_POLICY.md`, `CURRENT_EXECUTION_PLAN.md`, and the parallel-safe lane in `PHASE2_VERTICAL_SLICE_PLAN.md`:

1. inspect open PRs/CI first;
2. repair red work before starting new work;
3. merge only independently verified green and merge-ready work;
4. otherwise advance one highest-value authorized slice or qualifying parallel-safe sub-slice;
5. keep this status current when repository reality changes;
6. preserve accepted design/architecture decisions;
7. never convert portable CI into a substitute for required local Product Reality evidence.

## Repository posture

- visibility: **Public by deliberate operational choice**;
- IP: **Proprietary, all rights reserved**;
- public visibility does not grant an open-source license;
- default branch: `main`;
- substantive development: branch + pull request, not direct-to-main.
