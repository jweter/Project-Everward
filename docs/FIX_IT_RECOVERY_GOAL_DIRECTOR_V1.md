# Fix_It Recovery Goal Director — source-first projection (v1)

**Date:** 2026-11-17
**Status:** Implementation proposed; exact-head CI and UBT/native gameplay evidence are separately required. No claim that a whole starter zone is solvable or that mid-repair persistence works.
**Related:** #168 first playable, #159 native Product Reality, #378 owner Aura hard gate.

## DeepSeek review against actual source

The external design correctly identifies a gap between `FixItPlanner` and an actionable recovery goal, and recommends a read-only derived projection rather than another repair/quest authority. Actual repository inspection established:

1. `FixItDecision` already contains `stage`, `subsystem`, `integrity_before`, `target_integrity`, `material_required_kg`, `energy_required_j`, `time_required_s`, and the authoritative `reason`. Do not synthesize a second priority/rationale.
2. Both planners already implement survival-first prioritization and staged integrity bands. In practice the active Unreal actor calls **`FixItPlanner`**. `SelfRepairPlanner` supports 25/50/75/100% stages, while `FixItPlanner` uses a minimum useful 25%, then 50/75/100%.
3. Current `AFixItRuntimeActor` seeds Computation 18%, Thermal 22%, Sensors 35%, Propulsion 40% on an undamaged initial probe; the first `FixItPlanner` goal is Computation 18% → 25%. Material/energy amounts come from the current policy, never from hardcoded HUD text.
4. The selected target registry and existing bootstrap mining bridge are **not** a general `find_material_sources(material_id)` proof or universal mineability guarantee. There is no verified generic capability-unlock registry for every integrity threshold. No target- or action-specific claim is emitted by this slice.
5. `save_data.hpp` persists probe state, material inventory and integrity, but the **in-flight repair executor** (pending decision, elapsed time, already consumed mass/energy) is not persisted by that schema. Mid-repair save/load continuity is **not established**; rebuilding a goal from a loaded snapshot is different from resuming an executor transaction.
6. A pure per-snapshot projection cannot certify that future material sources are reachable, power can be replenished, controls work, or the full recovery scenario terminates. In particular, demoting unavailable actions cannot prove absence of a dependency cycle.
7. The active Unreal bridge and executor receipts live together in `AFixItRuntimeActor`. Therefore `GetRecoveryGoal()` is implemented on that actor, not on `UProbeSimulationAdapter`, avoiding a second owner or fabricated `InProgress` state.

## Implemented scope

- `src/simulation/include/everward/simulation/recovery_goal.hpp`: header-only deterministic `resolve_recovery_goal()` and `RecoveryGoalProjection`. No I/O, sim commands, resources mutation, target guesses or external dependencies.
- Identity: string `fix_it:<stage>:<subsystem>:<target-band-millionths>`; stable when inventory, progress and current integrity change during the same repair. It is not a mutable quest ID or a persisted transaction ID. It can repeat in a later recovery cycle; a future HUD should use event sequence for animations.
- State: `Blocked` (missing stored material/energy/time or fabrication), `Ready`, `InProgress`, `Interrupted`, `Completed`, `VerificationFailed`, `FullyRestored`, `NoRecommendation`, `AdvisoryOnly`. The resolver takes an actual executor receipt when one exists; **`Completed` requires both completion receipt and authoritative integrity ≥ planned target**.
- `AFixItRuntimeActor::GetRecoveryGoal()`: a new `BlueprintPure` read-only getter exposing the planner's actual target/rationale/cost, source-truth shortfalls and executor progress. Runtime repair selection, auto execution, save schema and simulation mechanics remain unchanged.
- The getter is intentionally **not** a new on-screen goal tracker. HUD PR #419 is already integrated separately; a later UI slice should consume this typed projection instead of re-parsing the verbose `GetFixItStatusSummary()` line.
- Testing: `everward_recovery_goal_tests` CTest target includes 12 cases. It checks actual current awakening ordering, stable identity, individual blockers, receipt precedence, false completion rejection, interruption, engineering-stage advisory, capability-gate fail-closed, a genuine `FixItRepairExecutor` material/energy/integrity cycle and next decision, and bounded planner-only repair sequence. Portable source contract in `tools/test_phase2_recovery_goal_contract.py`.

## What this does not certify

- A level's initial reachable mineral supply or physical tool-tip accessibility.
- Mining from arbitrary registered/scanned objects.
- Energy-generating time-to-recharge with real dynamic loads.
- A complete blocked-action solver, recovery goal pinning, player-selected repair, or visible component unlock.
- Correct actual in-engine HUD rendering, interaction, or UE native compilation.
- True `Fix_It` transactional mid-repair save/resume (must be a separate owner-coordinated slice because restart currently has no persistent executor receipt).
- Zero steady-state allocations, guaranteed UI cache hits or measured HUD frame time (the projection intentionally creates strings).
- Full Repair → Replacement → Upgrade → Redesign → Evolution gameplay. Advisory stages do not claim executable commands.

## Next player-visible slice (dependent on verified projection)

Add the typed `GetRecoveryGoal()` values to the compact default objective/optional Fix_It panel with one authoritative blocker and target band, without a new HUD control binding or imaginary material source. Then independently validate one real input sequence:

1. Damaged awakening and first recommendation;
2. physical target selection and scan/survey as necessary;
3. actual reachable tool alignment, accepted mining, authoritative named material delta;
4. Fix_It automatically starts only after sufficient resources; material/energy debited;
5. authoritative integrity advances to target with a completion receipt; next planner recommendation appears.

Require actual UE5.8 exact-head test/capture under #378. Never run or suggest another paid Aura test before ALL G01–G13 obligations are actually cleared.

## Future solvability validator — recommended separate engine slice

A meaningful validator must bound a *closed* recovery scenario state including (a) initial physical target geometry and manipulator staging, (b) authoritative material quantities and extraction rates, (c) sensor/mining capability gates, (d) power generation/allocation and repair consumption over time, (e) obstacles, exhaustion and selected alternatives, (f) save/recovery state. Explore reachable transitions with explicit budget and report **`UNKNOWN`** if the model is incomplete; return `UNSOLVABLE` only with a sound exhaustion proof under stated assumptions. A planner-only synthetic abundance test cannot substitute.

## Coordination / gates

One isolated source PR. No new launcher action, paid service, extra agent model, owner pause override or automatic merge authorization. Hosted CI/source checks, native compilation and actual gameplay are independent evidence grades. The verified acceptance basis remains issue #378 and the user-approved canonical recovery design.
