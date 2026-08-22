# Everward Project Status

This file is the durable operational continuation record for human and scheduled development. It records where active work should resume without replacing the authoritative roadmap, design pillars, ADRs, engine direction, or architecture documents.

## Current phase

**Phase 2 — One Probe: started.**

Phase 1 is complete. The decision-ready Unreal hardware artifact passes the executable Phase 1 exit gate, Unreal Engine remains the accepted production direction, and PR #68 has now created the first production Phase 2 implementation.

## What is now on `main`

PR #68, merged 2026-08-21, established the first real production runtime foundation:

- top-level Unreal Engine 5.8 project under `unreal/`;
- engine-independent C++20 authoritative simulation core under `src/simulation/`;
- canonical first-probe state including identity, position/velocity, mass, energy, temperature, storage, and basic capabilities;
- deterministic fixed-step movement integration and domain-event delivery;
- `UProbeSimulationAdapter` as the single Unreal-side caller of the simulation core;
- Blueprint-visible access to simulation tick, probe position, and velocity commands;
- CMake/CTest coverage for the production simulation core;
- GitHub Actions now compiles and tests the production simulation core on every PR in addition to all existing Phase 0/1 checks.

The production core compiled and passed its local CMake/CTest validation before push, and fresh independent GitHub CI run #143 completed successfully before PR #68 was merged.

PR #72 added the first command beyond movement, `ScanCommand`, entirely within `src/simulation/`:

- `SimulationCore::start_scan(target_id, duration_s)` validates a non-empty target, a positive duration, the `can_scan` capability, and that no scan is already in progress, then transitions probe state (`is_scanning`, `active_scan_target_id`, `scan_remaining_s`) and emits a `ScanStarted` domain event;
- scan progress is integrated on the same fixed-step `advance_wall_ticks` path used for movement, and emits a `ScanCompleted` domain event once the scan duration elapses;
- CMake/CTest coverage exercises validation failures, the started/completed event lifecycle, and that a new scan can begin after a prior one completes.

This intentionally does not yet touch `unreal/` — `UProbeSimulationAdapter` does not expose `ScanCommand` to Blueprint, and there is no embodied probe runtime scene yet for a scan command to be issued from. It also does not implement scan-result content (what a scan discovers); it proves the start/validate/complete lifecycle and timing only, matching `PHASE2_KICKOFF_SCAFFOLD.md`'s item 4.

PR #73 began power allocation mechanics (item 5), also entirely within `src/simulation/`:

- `SimulationCore::allocate_power(PowerSubsystem, watts)` validates a non-negative wattage and that the resulting combined allocation across sensors, propulsion, computation, and thermal subsystems does not exceed the probe's `power_capacity_w` budget, then sets that subsystem's allocation and emits a `PowerAllocationChanged` domain event;
- reallocating a subsystem replaces its existing share rather than accumulating on top of it; an over-capacity request is rejected and leaves existing allocations untouched;
- CMake/CTest coverage exercises validation failures (negative wattage, over-capacity requests), the happy-path allocation/event lifecycle across all four subsystems, reallocation, and the exact-headroom boundary case.

A follow-up PR (branch `claude/upbeat-lamport-q96q5d`) continues item 5 with the first consumption effect named as still pending: allocated power now draws down stored energy over simulated time.

- `SimulationCore::advance_wall_ticks` now integrates power consumption on the same fixed-step path as movement and scan progress: each step, total allocated wattage across all four subsystems (`total_power_allocated_w()`) draws `watts * elapsed_seconds` joules from `stored_energy_j`, clamped at zero rather than going negative;
- a new `EnergyDepleted` domain event fires exactly once on the transition from having stored energy to having none (mirroring the `ScanCompleted` transition-only pattern), not on every step energy happens to be draining;
- new CMake/CTest coverage exercises the no-op case (zero allocated power draws nothing across elapsed time), the happy-path partial drawdown against the expected `watts * seconds` joule figure, and the boundary case of advancing exactly enough ticks to land stored energy precisely at zero, including that continuing to advance afterward stays clamped at zero without re-emitting `EnergyDepleted`.

This also does not yet touch `unreal/` for the same reason as `ScanCommand` and the initial power-allocation slice: `UProbeSimulationAdapter` does not expose `allocate_power` to Blueprint, and there is no embodied probe runtime scene yet. It still does not model thermal load from allocated power or broader per-component operational/failure state beyond the allocation budget and this new energy-consumption effect.

A follow-up slice (branch `claude/upbeat-lamport-edtv0l`) closes the thermal-load gap named above: allocated power now also accumulates as waste heat in the probe's `temperature_k`.

- a new `thermal_capacity_j_per_k` field on `ProbeStateSnapshot` (default `2.5e6`, a plausible order-of-magnitude thermal mass for a ~2500 kg probe structure) converts joules of waste heat into a temperature rise;
- `SimulationCore::advance_wall_ticks` now also calls `integrate_thermal_load`, on the same fixed-step path as movement, scan progress, and energy consumption: each step, `total_power_allocated_w() * elapsed_seconds` joules are added to `temperature_k` as `delta_joules / thermal_capacity_j_per_k`;
- this intentionally treats all allocated power (across all four subsystems, including `Thermal`) as waste heat with no offsetting cooling effect, mirroring the same simplification already used for the stored-energy draw; a `Thermal` subsystem allocation that actively removes heat instead would be a new, currently-unspecified mechanical rule and is left for a deliberate follow-up rather than folded in here;
- no domain event is emitted by this slice: unlike `EnergyDepleted`, there is no currently-defined temperature threshold/transition to mark. Passive radiative cooling toward an ambient baseline and a temperature-limit/overheat response remain unimplemented and are the natural next follow-ups;
- new CMake/CTest coverage exercises the no-op case (zero allocated power leaves `temperature_k` unchanged across elapsed time), the happy-path single-step rise against the expected `watts * seconds / thermal_capacity_j_per_k` figure, and accumulation across multiple `advance_wall_ticks` calls while cross-checking that the parallel `stored_energy_j` draw from the same allocated wattage is unaffected by the new thermal integration.

This also does not yet touch `unreal/` for the same reason as the prior two slices, and does not implement passive cooling, temperature limits, or any behavioral response to overheating.

## Accepted production direction

**Unreal Engine is the accepted production engine direction.**

Authoritative references:

- `ENGINE_DIRECTION.md`
- `TECHNOLOGY_DECISIONS.md` TD-001
- `DECISION_LOG.md` ADR-0001
- `docs/PHASE2_KICKOFF_SCAFFOLD.md`

Godot material remains comparative/historical Phase 1 evidence only. Automation must not treat it as an alternate authorized production path.

## Current blocker

**No roadmap blocker.**

Residual rendering risk remains tracked from the Phase 1 Intel Iris Xe capture: the benchmark was strongly GPU-bound, used internal upscaling, and did not yet prove the final visual target on stronger hardware. Those are production-quality/performance risks, not blockers on Phase 2 implementation.

The Unreal production project itself has not yet been compiled/opened on the user's Windows Unreal installation after PR #68. That local Unreal compile is useful validation when convenient, but the next repository slice does not need to stop waiting for it unless a concrete Unreal build error is discovered.

## Exact continuation point

Resume with the next highest-value **Phase 2 — One Probe** slice.

The immediate target is to turn the new runtime foundation into the first visible embodied probe while preserving ADR-0002/ADR-0012 boundaries.

`ScanCommand` (sequence item 4 below) and power allocation with its energy-consumption and thermal-load effects (sequence item 5 below) are now implemented in `src/simulation/`, engine-independent and CTest-covered. Items 1–3 (the Unreal-side embodied probe runtime, transform-driving, and HUD read model) remain **not implemented**: they require compiling/running the Unreal project itself, which this scheduled run's environment cannot do (no Unreal Editor/UBT available to build or verify Unreal C++/Blueprint changes). Automation should not author unverifiable `unreal/Source/` changes; a run with Unreal build/verification capability should pick up items 1–3 next.

Recommended next sequence:

1. create a minimal runtime bootstrap in `unreal/` that instantiates exactly one probe presentation and exactly one `UProbeSimulationAdapter`;
2. drive the presented probe transform from the authoritative `src/simulation/` snapshot, with metres-to-centimetres conversion occurring only in the adapter/presentation boundary;
3. add a minimal inspect/HUD read model for mass, energy, temperature, storage, velocity, and simulation time;
4. ~~add the first real command path beyond movement: `ScanCommand` with validation plus `scan_started` / `scan_complete` events~~ — **done in `src/simulation/`**; still needs Blueprint/adapter exposure once item 1 exists;
5. begin power allocation and component-state mechanics — **in progress in `src/simulation/`**: `SimulationCore::allocate_power` validates and sets a per-subsystem (sensors/propulsion/computation/thermal) share of a total `power_capacity_w` budget and emits `PowerAllocationChanged`; allocated power now also draws down `stored_energy_j` over simulated time on the fixed-step path, clamped at zero, emitting `EnergyDepleted` on the depletion transition; allocated power now also accumulates as waste heat into `temperature_k` over the same fixed-step path via a new `thermal_capacity_j_per_k` field, with no offsetting cooling and no event (no threshold is defined yet). Still pending within this item: passive radiative cooling of `temperature_k` toward an ambient baseline, a temperature limit/overheat response, broader per-component operational/failure state (e.g. what happens once energy is fully depleted or the probe overheats — thrust/scan/compute degradation is not yet modeled), and Blueprint/adapter exposure of all three commands once item 1 exists;
6. continue until the Phase 2 gate is demonstrably true: **simply existing as the probe is compelling.**

The next highest-value engine-independent slice within item 5 is passive radiative cooling of `temperature_k` toward an ambient baseline (so temperature reaches an equilibrium under sustained load rather than climbing unbounded), following the same fixed-step-integration pattern as this slice. A temperature limit/overheat response and a behavioral response to full energy depletion (e.g. degrading `can_thrust`/`can_scan`) are reasonable follow-ups but are new, currently-unspecified mechanical rules and should each be scoped as their own deliberate slice rather than folded silently into any of the above.

Do not jump ahead to Phase 3 astronomy, Phase 4 industry, replication, aliens, combat, megastructures, or broad procedural content before the One Probe embodiment is functioning and testable.

## Fixed-step simulation rule

The substantive clock-drive defect identified after ADR-0012 was implemented in PR #68 rather than left as documentation-only work. Unreal uses a fixed-step accumulator and advances the otherwise-passive simulation core in whole deterministic steps; raw variable render-frame timing does not directly become mechanical simulation state.

PR #67 was closed as superseded by this implementation.

## Phase 2 production rules

- Simulation owns mechanical truth.
- Unreal consumes authoritative simulation state and submits commands through the single adapter boundary.
- `src/simulation/` must remain buildable/testable without Unreal dependencies.
- Canonical simulation units remain engine-independent; Unreal presentation conversion happens at the boundary.
- Deterministic headless execution remains required.
- Save data remains a versioned schema rather than blind Unreal object serialization.
- Large-scale simulation work must not become inseparable from rendered Actors/Components.

## Visual product constraint

Everward must not drift toward a primarily 2D, 2.5D, abstract-map, low-poly, deliberately quirky, or visually lightweight interpretation merely because it is easier to implement.

The target remains cinematic, immersive, high-fidelity 3D scientific realism. The player is the probe, and physical presence in a universe worth looking at is a first-class product requirement.

## Automation operating state

Scheduled development is governed by `AGENT_DEVELOPMENT_POLICY.md`.

Every run should:

1. inspect open PRs and CI first;
2. repair failed existing work before new roadmap work;
3. merge only work that is independently verified green and fully merge-ready;
4. otherwise advance one highest-value authorized slice of the current roadmap phase;
5. keep affected documentation current;
6. use accepted ADRs and this status file to avoid reopening settled decisions.

## Repository posture

- Repository visibility: **Public by deliberate operational choice**.
- Project IP posture: **Proprietary, all rights reserved**.
- Public visibility does not grant an open-source license.
- Default branch: `main`.
- Substantive autonomous development: branch + pull request; no direct-to-main development.

## Historical evidence

Detailed Phase 1 regression discoveries, benchmark evidence, mutation-test history, and failure/root-cause records belong in their existing proof files and `ERROR_RESOLUTION_LEDGER.md` rather than accumulating here.

This file should stay concise and current: **where we are, what blocks us, what decision is settled, and what work is authorized next.**
