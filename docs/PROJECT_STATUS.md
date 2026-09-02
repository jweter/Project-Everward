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

Current `main`: `b63fa54eb1a60143e8374b075847953bad1c56f4`, after Slice 7 gained deterministic target cycling (#150) on top of nearest-target selection and range telemetry (#131). The evolving-sensorium-audio foundation (#122) and the Prime Generation-1 body blockout (#123, Slice 5) are folded in below alongside the manipulator arm work.

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

### Physical target selection and range telemetry foundation (Slice 7)

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 7 ("object selection and physical
interaction") minimum interactions begin with "select a nearby physical
target" and "display range/relative motion" -- until the previous pass the
only physical target the probe could act on at all was
`CommandMineBootstrapTarget`'s single hardcoded body ID, with no generic
selection or range/closing-speed concept anywhere in the simulation.
`target_selection.hpp` added that as pure, engine-independent read-side math
over the same registered `StaticSphereBody` list
`software_policy.hpp`'s swept contact solver already consumes
(`surface_range_to_body`, `closing_speed_to_body`,
`find_nearest_selectable_target`, `select_target_telemetry`, the last two
fail-closed for no match rather than guessing or keeping a stale selection).

This pass wires that math into runtime state, the adapter, and a minimal
HUD/input surface:

- `ProbeRuntime` now owns authoritative selection state alongside
  `static_bodies_`: `select_nearest_target`, `select_target`,
  `clear_target_selection`, and `selected_target_status()` (recomputed live
  on every call, so a since-deregistered selection reports unselected
  without a separate mutating call); `DamageAwareProbeRuntime` forwards all
  four rather than duplicating the registry or math;
- `UProbeSimulationAdapter` exposes `GetSelectedTargetStatus()` and
  `CommandSelectNearestTarget` / `CommandSelectTarget` /
  `CommandClearTargetSelection` (new `ProbeTargetSelectionBridge.cpp`,
  mirroring the existing `ProbeMiningBridge.cpp` split);
- `T` selects the nearest registered body within a configurable range
  (default 500 m); the always-visible telemetry panel gained a `TARGET` row
  reporting the selected id, surface range, and closing speed, or a muted
  "no selection" prompt; the `F1` controls reference documents the binding.

This still does not assume the still-Product-Reality-pending
contact/manipulator collision behavior is correct and changes none of it --
selection only reads probe position/velocity and the existing body
registry and mutates no other authoritative state, so it remains within the
parallel-safe lane.

**Status: implemented, Product Reality pending.** See
`PHASE2_TARGET_SELECTION_TEST.md`.

Two further parallel-safe sub-slices then landed on top of this foundation:

- **#149** added `find_next_selectable_target()`, an engine-independent
  nearest→farthest ordering primitive over the same registered-body list,
  with deterministic tie-breaking by registration order;
- **#150** wired that ordering into authoritative `cycle_next_target_selection()`
  state on `DamageAwareProbeRuntime`: with no current selection it picks the
  nearest eligible body; repeated calls advance nearest→farthest and wrap
  from farthest back to nearest; a stale/out-of-range current selection
  restarts at nearest; no eligible body clears selection fail-closed. The `T`
  key binding now calls the new `UProbeSimulationAdapter::CommandCycleTarget()`
  instead of select-nearest, so `T` cycles through targets rather than only
  reselecting the closest one. See `PHASE2_TARGET_CYCLING_TEST.md`, which
  notes the current scene has only one registered physical body, so
  multi-target wraparound cannot be visually verified locally until Slice 8
  adds more targets.

**Status: implemented, Product Reality pending.** No approach/reach/grasp
mechanics exist yet -- those remain later Slice 7 sub-slices.

A fourth parallel-safe sub-slice then added the visual selection indicator
that gap previously called out: `AEverwardPhase2TestEnvironment` gained
`RefreshTargetSelectionHighlight()`, which reads the same authoritative
`GetSelectedTargetStatus()` the HUD `TARGET` row already renders and
retints the registered physical body's existing dynamic material (a bright
cyan, more metallic highlight in place of its default regolith-rock look)
whenever that body is the selected target, reverting the moment the
selection clears or moves elsewhere. No new simulation state, adapter
method, or input binding was added -- this is a presentation-only retint of
an already-existing mesh, so it stays in the parallel-safe lane the same
way the underlying selection telemetry did. See
`PHASE2_TARGET_VISUAL_INDICATOR_TEST.md`.

**Status: implemented, Product Reality pending.** No Unreal Editor/UBT
build was available in this sandbox to compile-verify this change; it
follows the exact `UMaterialInstanceDynamic` pattern already compiling
elsewhere in `EverwardPhase2TestEnvironment.cpp`, and the new
`tools/test_phase2_target_selection_surface.py` case passed, but the next
local Unreal Product Reality pass should specifically confirm the project
still compiles under UBT before relying on this further.

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

All three Slice 6 sub-slices (foundation mechanics, joint-articulation input/HUD, visible geometry) are now implemented, and Product Reality evidence is outstanding for all three.

### Manipulator arm/body and arm/environment collision (Slice 6 follow-up)

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 6 player-visible result list includes "collision does not allow impossible penetration through the probe body." Every arm mesh is still built through `EverwardProbePawn.cpp`'s `ConfigureMesh`, which sets `ECollisionEnabled::NoCollision`, so Unreal itself still cannot block an arm mesh from passing through another mesh. Rather than adding reactive Unreal collision to a kinematic, attachment-driven rig (which UE does not sweep automatically for attached children), this makes the requirement hold the same way `manipulator.hpp`'s header comment says it should: Unreal only ever renders whatever `ManipulatorRig` produces, so an intersecting pose is now simply unreachable in the authoritative simulation, the same way an out-of-range joint angle already was.

- new engine-independent `manipulator_hull_contact.hpp`: forward kinematics for the elbow/wrist pivots (converted from `EverwardProbePawn.cpp`'s centimeter constants, reusing `rotate_local_contact_offset`'s roll/pitch/yaw convention so it matches Unreal's `FRotator` composition order), checked against the same `ProbeCompoundCollisionEnvelope` five-sphere hull `software_policy.hpp` already sweeps external contact against — no second hull shape invented;
- `ManipulatorRig` takes an optional `SelfCollisionGuard` callback (default: none, so every existing test and call site is unaffected); when a commanded joint's next slewed step would intersect the hull, that step is held rather than applied, exactly scoped to the fully-deployed, not-mid-deploy/stow regime `command_joint_target_degrees` already requires — deploy/stow's own fold motion is deliberately left unguarded so this coarse hull approximation cannot deadlock it;
- this run closed the remaining arm/environment gap the same way: `manipulator_pose_intersects_environment` places the same elbow/wrist samples in world space (probe position plus `rotate_local_contact_offset` by the probe's current attitude) and checks them against every body `ProbeRuntime::static_bodies()` currently has registered — a static overlap test, not the swept/velocity-resolved contact response `software_policy.hpp` performs for the probe hull itself, so this stays independent of that still-pending behavior's correctness (PHASE2_VERTICAL_SLICE_PLAN.md's parallel-safe lane); `make_environment_collision_guard` takes the probe pose and registered-body list as injected callables so the guard always reasons about live state, never a snapshot fixed at rig-construction time; `make_combined_collision_guard` folds the hull and environment guards into the single `SelfCollisionGuard` `ManipulatorRig` accepts;
- `ProbeSimulationAdapter.cpp` wires `make_combined_collision_guard(make_hull_self_collision_guard(), make_environment_collision_guard(...))` into its `ManipulatorRig`, with the environment guard's callables reading `Core`'s live snapshot/registered bodies on every call;
- unit coverage (`everward_manipulator_hull_contact_tests`, 16 cases): the idle fully-deployed rest pose does not false-positive against the hull; folding the shoulder to its -90 degree limit does intersect the hull; a rig built with the real hull guard cannot reach that folded pose (stops short, and the pose it stops at is verified non-intersecting); an analogous case for a registered external body instead of the hull; environment intersection is false with no registered bodies, detects a body colocated with the wrist, ignores a distant body, and tracks the probe's current world pose rather than a fixed one; deploy/stow still completes normally with either guard installed; an ordinary 45 degree shoulder swing with a full elbow bend still reaches its exact commanded target; the combined guard rejects a pose either sub-guard rejects and matches unguarded behavior when both guards are empty. All 11 `src/simulation` ctest suites pass (Debug and Release), and the existing `tools/test_phase2_manipulator_arm_surface.py` source-contract test still passes unchanged against the new construction call.

**Status: implemented, Product Reality pending** for both the self-collision and environment-collision behavior (confirming a commanded fold-in, and a commanded pose that would sweep an arm into the registered physical scan target, both visibly stop rather than clip, in the actual UE build). No Unreal Editor/UBT build of `ProbeSimulationAdapter.cpp` was available in this sandbox to compile-verify the adapter wiring itself; the change there follows the exact accessor patterns (`Core->snapshot()`, aggregate-initializing `ProbeWorldPose`/`StaticSphereBody`) already used and compiling elsewhere in that file, and the source-contract test above passed, but the next local Unreal Product Reality pass should specifically confirm the project still compiles under UBT before relying on this further.

### Manipulator reach telemetry — "align a manipulator" (Slice 7)

`PHASE2_VERTICAL_SLICE_PLAN.md`'s Slice 7 loop is
`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`.
Target selection/cycling/highlighting already covered `detect -> select`,
and the manipulator arms already existed on their own, but nothing
previously connected the two: there was no way to tell whether a deployed
arm was actually close enough to the selected target to do anything with
it. This pass adds exactly that minimum interaction and nothing past it —
**read-only telemetry, no new grasp/attach/dock physics**:

- new engine-independent `manipulator_reach.hpp`: `manipulator_reach_status()`
  reuses `manipulator_hull_contact.hpp`'s existing `manipulator_arm_contact_samples()`
  for the wrist's probe-local position and the same `rotate_local_contact_offset`
  local-to-world convention `software_policy.hpp`'s probe-vs-body contact and
  the arm/environment collision guard already use, then reuses
  `target_selection.hpp`'s `surface_range_to_body()` against the same
  registered `StaticSphereBody` list the `TARGET` HUD row already reads —
  no second forward-kinematics model, world-placement convention, or range
  formula invented;
- a fixed 2.0 m reach envelope (`ManipulatorReachEnvelopeMeters`), chosen
  relative to the arm's own geometry: the wrist-to-tool-tip extension is
  0.79 m and the shoulder-to-wrist arm length is 3.30 m, so 2.0 m stays
  comfortably inside full extension while leaving slack for the tool tip and
  final alignment rather than requiring the wrist itself to touch the
  surface;
- fails closed (no result) rather than fabricating a reading whenever there
  is no selected target, the selected target has since been deregistered, or
  the queried arm is not in the same fully-deployed/not-mid-transition
  steady state `command_joint_target_degrees` itself already requires before
  accepting a joint command;
- a `DamageAwareProbeRuntime`-based convenience overload mirrors
  `target_cycle_runtime.hpp`'s `cycle_next_target_selection()` pattern, since
  neither `ProbeRuntime` nor `DamageAwareProbeRuntime` owns manipulator arm
  state (that remains the Unreal adapter's separately-owned `ManipulatorRig`,
  the same architecture `GetManipulatorArmStates()` already reads directly);
- `UProbeSimulationAdapter::GetManipulatorReachStatus(ArmId)` combines
  `Core`'s live pose/target-selection state with `Manipulators`' live arm
  state and recomputes on every call, following the exact pattern
  `GetSelectedTargetStatus()` and `GetManipulatorArmStates()` already use;
- the existing `M` manipulator HUD page gains a `REACH` row for whichever
  arm the page currently has selected, shown only once a target is actually
  selected, reading "IN REACH" / "OUT OF REACH // *X* M REMAINING" once the
  arm is fully deployed or a muted "ARM NOT DEPLOYED" explanation otherwise
  — no new input binding was needed, since arm/target selection state
  already existed;
- deterministic/source-contract coverage: new `everward_manipulator_reach_tests`
  (in-reach, out-of-reach with expected remaining distance, no-target-selected,
  arm-not-deployed, arm-mid-deploy, arm-mid-stow, a stale/deregistered
  selected-target id, live pose tracking, and the runtime overload) plus
  `tools/test_phase2_manipulator_reach_surface.py` proving the math is
  engine-independent, fails closed, and is actually wired into the
  adapter/HUD rather than merely present alongside them.

**Status: implemented, Product Reality pending.** No Unreal Editor/UBT
build was available in this sandbox to compile-verify the
`ProbeSimulationAdapter.h`/`.cpp` or `EverwardHUD.cpp` changes; they follow
the exact accessor and HUD-row patterns already compiling elsewhere in
those files. See `PHASE2_MANIPULATOR_REACH_TELEMETRY_TEST.md`. This does not
advance Slice 7's completion gate by itself: approach-as-motion, grasp,
dock, move, and release still do not exist.

### Manipulator grasp — "grasp or dock with a simple object" (Slice 7)

The first Slice 7 interaction past read-only telemetry: `manipulator_grasp.hpp`'s
`attempt_grasp_selected_target()` reuses `manipulator_reach_status()`'s
existing `in_reach` result as the sole proximity gate — a grasp attempt
succeeds exactly when the manipulator page's REACH row would read "IN
REACH", never a second notion of "close enough". `ManipulatorRig` gains
`begin_grasp()`/`release_grasp()`, mirroring `attach_tool()`/`detach_tool()`'s
shape: deployed-only, one held object at a time, and stowing a still-
grasping arm is rejected the same way stowing with a tool attached already
is. `UProbeSimulationAdapter::CommandGraspSelectedTarget()`/
`CommandReleaseGraspedTarget()` expose the two commands; the manipulator
page's existing per-arm status line (both there and on the always-visible
telemetry panel) now appends `// HOLDING <target id>` while grasping; `F`
toggles grasp/release on whichever arm the manipulator page currently has
selected.

**Status: implemented, Product Reality pending.** No `move` mechanics exist
yet — a grasped object does not follow the probe or arm — and this does not
advance Slice 7's completion gate by itself. See
`PHASE2_MANIPULATOR_GRASP_TEST.md`. No Unreal Editor/UBT build was available
in this sandbox to compile-verify the `ProbeSimulationAdapter.h`/`.cpp`,
`EverwardHUD.cpp`, or `EverwardPlayerController.h`/`.cpp` changes; they
follow the exact accessor/command, HUD-row, and key-binding patterns already
compiling elsewhere in those files. The next local Unreal Product Reality
pass should specifically confirm the project still compiles under UBT
before relying on this further, and should exercise step 21 below.

### Manipulator move — read-only wrist-follow math foundation (Slice 7)

The same incremental pattern `target_selection.hpp` and
`manipulator_reach.hpp` already used: a pure, engine-independent math
foundation lands first, then a later sub-slice wires it into authoritative
runtime state, the adapter, and the Unreal-side actor. `manipulator_move.hpp`'s
`grasped_target_position()` reports, read-only, where a currently grasped
target should be positioned — the grasping arm's wrist world position —
reusing the exact same forward-kinematics and local-to-world convention
`manipulator_reach_status()` already establishes rather than inventing a
second one. It returns `nullopt` whenever the queried arm holds nothing.

This did **not** yet update the registered `StaticSphereBody`'s own
authoritative `center_m` when it first landed; that wiring is the subject of
the next pass below.

A follow-on pass wires that read-only math into authoritative state and
Unreal presentation. `ProbeRuntime`/`DamageAwareProbeRuntime` gain
`update_static_sphere_body_position()` — the single mutation point for a
registered body's `center_m`, so every existing reader (contact, target
selection, reach) automatically reflects a carried body's current position
without a second position path. `UProbeSimulationAdapter::TickComponent()`
calls `grasped_target_position()` for both arms after
`Manipulators->advance()` each fixed step and feeds a non-empty result
through that mutation point. A new `GetStaticBodyPositionMeters()` accessor
reads a registered body's current position back out, and
`AEverwardPhase2TestEnvironment::RefreshScanTargetPosition()` (new, called
from `Tick()` alongside the existing `RefreshTargetSelectionHighlight()`)
mirrors it onto the already-existing `ScanTargetMesh`/`ScanTargetLabel`
components every tick — no new Unreal-owned motion/physics system.

**Status: implemented, Product Reality pending.** New deterministic
coverage in `everward_manipulator_move_tests`: the original grasp-position
math tests (empty grasp returns `nullopt`, wrist-world-position matches
`manipulator_arm_contact_samples()` exactly at the probe origin, the result
translates with the probe, the query stays scoped to the actually-holding
arm, `release_grasp()` clears the result, and the runtime overload matches
the free function) plus new coverage for
`update_static_sphere_body_position()`: it moves only the matching
registered body's `center_m` and leaves everything else about it unchanged,
an unregistered id is a no-op rather than a fabricated registration, and an
end-to-end scenario proves `selected_target_status()`/`static_bodies()` both
report a held body's new wrist-following position once it is written
through this mutation point. See `PHASE2_MANIPULATOR_MOVE_TEST.md` and
`tools/test_phase2_manipulator_move_surface.py`. No Unreal Editor/UBT build
was available in this sandbox to compile-verify the
`ProbeSimulationAdapter.h`/`.cpp` or `EverwardPhase2TestEnvironment.h`/`.cpp`
changes; they follow the exact accessor/component-mutation patterns already
compiling elsewhere in those files (`GetSelectedTargetStatus()`,
`RefreshTargetSelectionHighlight()`). The next local Unreal Product Reality
pass should specifically confirm the project still compiles under UBT
before relying on this further, and should exercise the new step in the
sequence below.

### Mining routes extracted mass through authoritative storage

The scan-to-mining loop's `CommandMineBootstrapTarget` bridge previously
accumulated extracted material only in an adapter-local counter
(`BootstrapExtractedMaterialKilograms`) and never wrote it into `ProbeRuntime`'s
authoritative `storage_used_kg`, even though the always-visible telemetry
HUD's STORAGE row reads exactly that field. The general cargo readout therefore stayed at
0% regardless of how much had actually been mined, while the separate mining
status widget correctly showed the recovered mass — a truth split between two
HUD elements that should agree. `SimulationCore`/`ProbeRuntime`/
`DamageAwareProbeRuntime` now expose `add_stored_material_kg`, the sole
mutation boundary for collected material, and the bridge routes every
accepted mining cycle through it instead.

**Status: implemented, Product Reality pending.** No behavior other than
where extracted mass is recorded changed; `docs/PHASE2_SCAN_TO_MINING_TEST.md`
now also checks the always-visible STORAGE mass and percentage after mining.

### Human-readable HUD and dedicated controls reference

The user-provided 2026-08-30 current-build captures confirm the prior HUD is a
Product Reality blocker: it exposes the correct categories, but most live
telemetry, system explanations, and manipulator bindings are too small to read
at normal laptop distance. This pass responds at the presentation layer:

- viewport-responsive layout sizing and Unreal's medium HUD font with a
  minimum readable text scale;
- wider, more opaque live panels with larger row spacing and shortened compact
  status explanations;
- exact authoritative storage mass/capacity/percentage in the always-visible
  telemetry panel;
- a short persistent entry bar for `F1` Controls, `Tab` Systems, and `M`
  Manipulator rather than one miniature all-bindings line;
- a dedicated `F1` controls reference grouped by Flight + Camera, Systems, and
  Manipulator + Mining, following the successful control-learning structure
  observed in *Delta-V: Rings of Saturn* without copying its visual styling;
- a materially larger/higher world-space resource instruction label;
- suppression of identical simultaneous command/automation rejection text.

**Status: implemented, Product Reality pending.** See
`PHASE2_HUD_LEGIBILITY_AND_CONTROLS_TEST.md`. Static source-contract coverage
protects the intended surface, but only the next local Unreal capture can
prove font rendering, clipping, overlap, and normal-distance readability.

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
- physical target selection, nearest→farthest cycling, range/closing-speed telemetry, and a mesh-retint visual selection indicator, wired into `ProbeRuntime`/`DamageAwareProbeRuntime`, the adapter, and a minimal HUD/input/presentation surface (Slice 7 foundation; implemented, Product Reality pending);
- manipulator arm deploy/stow/joint/tool foundation, joint articulation input and a dedicated manipulator HUD page, visible shoulder/arm/tool-head geometry, and arm/body + arm/environment collision guards (Slice 6; implemented, Product Reality pending);
- manipulator reach telemetry connecting target selection to the arms: whether the currently selected arm's wrist is within a fixed reach envelope of the selected target's surface, reported on the existing manipulator HUD page (Slice 7 "align a manipulator"; implemented, Product Reality pending);
- manipulator grasp: an arm within reach of the selected target can grasp/release it (`F`), gated by the exact same reach result and rejecting stow while still holding something (Slice 7 "grasp or dock with a simple object"; implemented, Product Reality pending; no `move` mechanics yet);
- manipulator move: a currently grasped target's registered position now follows the holding arm's wrist every fixed step through a single authoritative mutation point (`update_static_sphere_body_position()`), with the Unreal-side scan-target mesh/label mirroring that same position each tick (Slice 7 "move"; implemented, Product Reality pending; no `release`-with-consequence beyond leaving the object where it currently is);
- canonical Prime Probe A / Scientific Explorer reference package with provenance validation;
- explicit versioned save architecture rather than blind Unreal object serialization.

## Exact next local UE 5.8 Product Reality pass

Use the exact passed build and validate the accumulated Phase-2 chain in order:

Before the accumulated mechanics sequence, run
`PHASE2_HUD_LEGIBILITY_AND_CONTROLS_TEST.md`. If ordinary telemetry or the F1
reference is still unreadable, clipped, or overlapping, stop and repair the
HUD before attempting later mining/contact acceptance.

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
15. deploy an arm, fold the shoulder toward its limit and confirm the arm visibly stops short of passing through the probe's own hull rather than clipping through it; then approach the registered physical scan target with an arm deployed and command a joint pose that would sweep the arm into it, and confirm that motion also stops short instead of clipping into the target;
16. press `T` within 500 m of the registered physical target and verify the telemetry panel's `TARGET` row shows its id, surface range, and closing speed that track live as you approach/retreat, and that the target's own mesh visibly retints to its selected highlight color at the same moment (and reverts the moment selection clears), then press `T` again out of range and verify a clear rejection rather than a stale reading or stale highlight (`PHASE2_TARGET_SELECTION_TEST.md`, `PHASE2_TARGET_VISUAL_INDICATOR_TEST.md`);
17. with the target still selected, press `T` again and confirm the current build's single registered physical body keeps the same target selected (cycling wraps to itself); note that multi-target nearest→farthest wraparound cannot be visually verified until Slice 8 adds more registered bodies (`PHASE2_TARGET_CYCLING_TEST.md`);
18. with a target still selected, open the manipulator page (`M`) and confirm a `REACH` row appears for the currently selected arm, reading "ARM NOT DEPLOYED" while stowed and switching to a live "IN REACH" / "OUT OF REACH // remaining distance" reading once deployed that tracks smoothly as the probe approaches/retreats and updates correctly after `N` switches the selected arm (`PHASE2_MANIPULATOR_REACH_TELEMETRY_TEST.md`);
19. record visual overlap, clipping, unreadable telemetry, implausible contact, damage mismatch, or control confusion as Product Reality evidence;
20. repeat the embodiment/HUD/control/clunkiness/movement/automation/desire-to-continue ratings against the first-run baseline;
21. with the arm out of reach of the selected target, press `F` and confirm the grasp is rejected; approach until the REACH row reads "IN REACH", press `F`, and confirm the arm's status line (both the manipulator page and the always-visible telemetry panel) now reads `// HOLDING <target id>`; attempt to stow that arm and confirm it is rejected; press `F` again to release, confirm `HOLDING` disappears, and confirm the arm now stows normally (`PHASE2_MANIPULATOR_GRASP_TEST.md`);
22. while an arm is still holding the target, translate the probe and separately nudge the holding arm's joints (`,`/`.`) and confirm the `SCAN-001` mesh and its label visibly follow both the probe's motion and the arm's articulation rather than staying at their original position, that the `TARGET`/`REACH` rows keep reading correctly against the now-moved body, and that releasing (`F`) leaves the mesh at its current position rather than snapping back (`PHASE2_MANIPULATOR_MOVE_TEST.md`).

A failure in orientation/control or physical contact outranks later roadmap work. A damage-layer failure blocks Slice 4 completion. Portable CI is not a substitute for this test.

## Authorized next development

Priority order:

1. complete the accumulated local Phase-2 Product Reality pass above;
2. repair any failed orientation, subsystem, contact, or damage behavior before building on it;
3. **Slice 6 — articulated manipulator arms**: mechanics, joint-articulation HUD, visible geometry, and arm/body + arm/environment collision are all now implemented (every arm mesh still has `ECollisionEnabled::NoCollision`, but a self- or environment-intersecting pose is unreachable in the authoritative `ManipulatorRig` regardless) — Slice 6 is not complete until the local Product Reality pass above is recorded across its three test scripts plus the new collision behavior (step 15);
4. **Slice 7 — object selection and physical interaction**: nearest-target selection, range/closing-speed telemetry, nearest→farthest target cycling, a visual selection indicator on the target's own mesh, manipulator reach telemetry ("align a manipulator"), manipulator grasp/release ("grasp or dock with a simple object"), and manipulator move (a held target's registered position and its Unreal-side mesh/label now follow the holding arm's wrist) are implemented (Product Reality pending, step 16/17/18/21/22 below); no approach-as-motion or release-with-consequence beyond leaving the object where it currently is exist yet — those are the next Slice 7 sub-slices;
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
