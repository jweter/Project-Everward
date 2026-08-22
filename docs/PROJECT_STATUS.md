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

A follow-up slice (branch `claude/upbeat-lamport-jb8zbf`) closes the passive-cooling gap named above: `temperature_k` now cools passively back toward an ambient baseline instead of only ever rising under load.

- two new `ProbeStateSnapshot` fields: `ambient_temperature_k` (default `293.15`, matching the probe's initial `temperature_k` so a freshly constructed probe starts in thermal equilibrium with its environment before any power is allocated) and `passive_cooling_w_per_k` (default `2.0`, an order-of-magnitude placeholder for a modest radiator/conduction pathway, not a Stefan-Boltzmann radiative model);
- `integrate_thermal_load` now models Newtonian cooling combined with the existing constant waste-heat input: `dT/dt = (heating_w - passive_cooling_w_per_k * (T - ambient_temperature_k)) / thermal_capacity_j_per_k`, solved in closed form (`T(t) = T_eq + (T0 - T_eq) * exp(-k*t)`, `T_eq = ambient_temperature_k + heating_w / passive_cooling_w_per_k`, `k = passive_cooling_w_per_k / thermal_capacity_j_per_k`) rather than by a fixed-step Euler update, so the result is exact and step-size-independent for any elapsed duration on the same `advance_wall_ticks` path used for movement, scanning, energy consumption, and the prior pure-heating thermal model;
- under sustained allocated power, `temperature_k` now asymptotically approaches a finite equilibrium (`ambient_temperature_k + total_power_allocated_w() / passive_cooling_w_per_k`) instead of climbing unbounded; with zero allocated power, `temperature_k` decays back toward `ambient_temperature_k` from either side rather than staying frozen at whatever value heating last left it at;
- if `passive_cooling_w_per_k` is ever `0`, the integration falls back to the prior pure-heating-only accumulation (no divide-by-zero, no cooling pathway) to keep the previous behavior available as a degenerate configuration;
- new CMake/CTest coverage exercises the no-op case (probe starts exactly at its own ambient baseline with no power allocated: zero net movement), the happy-path warm-toward-equilibrium case under sustained power (asserted against the closed-form equation computed independently in the test, not by re-deriving the implementation's own code), step-size independence (two fixed steps totalling 5 seconds land at the same temperature as one 5-second step, which a naive fixed-step Euler update would not guarantee), and pure passive cooling with zero allocated power (a probe left warmer than ambient cools back toward it, monotonically and without overshooting past it, purely from elapsed time).
- this same slice also repaired a pre-existing latent bug in `EnergyConsumption`'s exact-depletion boundary test (`exact_depletion_ticks` truncated a fractional tick instead of rounding up, leaving `stored_energy_j` a fraction of a joule above zero so `EnergyDepleted` never fired); see `ERROR_RESOLUTION_LEDGER.md` for the root cause and verification. No `SimulationCore` production behavior changed by this repair — only the test's own tick-rounding.

This also does not yet touch `unreal/` for the same reason as the prior three slices, and does not implement a temperature limit/overheat response or any behavioral consequence of high temperature — both remain open, currently-unspecified follow-ups.

A follow-up slice (branch `claude/upbeat-lamport-eyesqm`) closes the temperature-limit/overheat gap named above: `temperature_k` now has a behavioral consequence once it crosses a defined limit, rather than only ever being a readable number.

- a new `max_operating_temperature_k` field on `ProbeStateSnapshot` (default `373.15`, an order-of-magnitude placeholder for a spacecraft electronics thermal limit, not a modeled material or component-specific failure point) defines the threshold, and a new `is_overheated` flag records the probe's current lockout state;
- a new `integrate_overheat_response` step runs on the same fixed-step `advance_wall_ticks` path as movement, scanning, energy consumption, and thermal load, after `temperature_k` is updated for that step: crossing at or above `max_operating_temperature_k` sets `is_overheated`, clears `can_scan` and `can_thrust`, and emits a new `OverheatStarted` domain event; dropping back below the threshold restores both capability flags and emits a new `OverheatEnded` domain event; this is edge-triggered on `is_overheated` (mirroring `EnergyDepleted`'s transition-only pattern), so remaining above or below the threshold across further steps does not re-emit either event;
- while overheated, `start_scan` and `set_velocity_mps` reject exactly as they already do for any other `can_scan`/`can_thrust`-gated command — no new rejection path was needed, only the existing capability flags being driven by a new cause;
- `can_scan`/`can_thrust` currently have no other source of truth than this response, so unconditionally restoring both to `true` on recovery is safe today; a future independent failure/operational-state system that can also disable these flags would need to reconcile with this response rather than assume it owns them exclusively — noted in `core.hpp` for the next slice that adds one;
- new CMake/CTest coverage exercises the no-op case (no allocated power never approaches the limit), the happy-path crossing under sustained maximum allocated power (computed from the same closed-form cooling equation `integrate_thermal_load` itself uses, rounded up to a whole tick as `EnergyConsumption`'s own exact-boundary test does) including that scan/thrust commands are rejected while overheated and that `OverheatStarted` does not re-fire on a subsequent step spent still above the limit, and the recovery path (deallocating power lets `temperature_k` decay back below the limit, restoring capabilities and firing `OverheatEnded` exactly once).

This also does not yet touch `unreal/` for the same reason as the prior four slices, and does not implement a behavioral response to full energy depletion (`stored_energy_j` reaching zero) or any broader per-component operational/failure state — both remain open, currently-unspecified follow-ups, as previously noted.

A follow-up slice (branch `claude/upbeat-lamport-knlfp2`) closes the energy-depletion gap named above: `stored_energy_j` reaching zero now also has a behavioral consequence, mirroring the overheat lockout precedent while explicitly reconciling with it rather than assuming the two causes are independent.

- a new `is_energy_depleted` flag on `ProbeStateSnapshot` is set the moment `stored_energy_j` transitions from having stored energy to having none (the same `previous_energy_j > 0.0 && remaining_energy_j <= 0.0` transition that already fires the existing `EnergyDepleted` domain event, reused rather than duplicated with a second event for the same transition);
- `can_scan`/`can_thrust` are now derived once per `advance_wall_ticks` step from **both** lockout causes (`is_overheated || is_energy_depleted`) via a new `refresh_capability_lockouts()` step that runs after `integrate_power_consumption` and `integrate_overheat_response`, rather than either response directly writing `true`/`false` to those flags. This is the deliberate scoping the prior continuation note called for: the two causes now correctly stack — one clearing (e.g. `temperature_k` cooling back below `max_operating_temperature_k`) does not wrongly restore capabilities while the other is still active (e.g. `stored_energy_j` still at zero), and vice versa;
- unlike `is_overheated`, `is_energy_depleted` intentionally has no recovery/restore branch: no mechanic exists yet that raises `stored_energy_j` back above zero (no charge/generation slice has been built), so a `0 -> >0` transition is currently unreachable and was not coded as dead, untestable branches. A future energy-recharge slice should add that transition (and its own recovery event) when it exists;
- while depleted, `start_scan` and `set_velocity_mps` reject through the same existing capability-gated rejection path already used for overheat and, before that, `can_scan`/`can_thrust` in general — no new rejection code was needed;
- new CMake/CTest coverage exercises the no-op case (no allocated power never depletes), the happy-path depletion-triggers-lockout case (deliberately using a wattage below the thermal equilibrium that would ever cross `max_operating_temperature_k`, so it stays isolated to energy alone) including that scan/thrust commands are rejected, and a combined-lockout interaction case: sustained maximum power drives the probe into **both** `is_overheated` and `is_energy_depleted`, then deallocating power lets `temperature_k` cool back below the limit (firing `OverheatEnded` exactly once) while `stored_energy_j` stays at zero — proving `can_scan`/`can_thrust` correctly remain locked throughout, not just implemented and cleared as the last-applied side effect would happen to have left them.

This same slice also found and fixed a significant, unrelated pre-existing defect discovered while adding the above coverage: `.github/workflows/foundation.yml`'s `cmake -S src/simulation -B build/simulation -DCMAKE_BUILD_TYPE=Release` step defines `NDEBUG`, which silently compiles every `assert()` in `simulation_core_tests.cpp` into a no-op — the CTest step has been reporting success unconditionally, regardless of whether any assertion actually held, for every simulation-core slice since PR #68. See `ERROR_RESOLUTION_LEDGER.md`, 2026-08-22, for the full root cause, the fix (`#undef NDEBUG` before `#include <cassert>`, verified end-to-end against the exact CI `cmake`/Release command by confirming CTest now genuinely fails on a deliberately broken assertion and genuinely passes once reverted), and a second, previously-masked test defect this uncovered and also fixed in the same commit (the `EnergyConsumption` boundary test's assumed two-event outcome no longer held once `integrate_overheat_response` existed, at that test's original wattage).

This also does not yet touch `unreal/` for the same reason as the prior five slices, and does not implement any energy-recharge/generation mechanic, per-component operational/failure state beyond the two probe-wide lockouts (overheat and energy depletion), or software policy state.

A follow-up slice (branch `claude/upbeat-lamport-vkr9da`) closes the energy-recharge gap named above: `stored_energy_j` can now be recharged by a passive generation source instead of being permanently one-way once depleted, completing option (a) of the two next-slice options this file previously named.

- a new `energy_generation_w` field on `ProbeStateSnapshot` (default `0.0`) models a constant passive power supply — deliberately RTG-style (radioisotope thermoelectric generator) rather than solar, since solar generation would need the star-distance/irradiance model that does not exist yet — configured via a new `SimulationCore::set_energy_generation_w(watts)` hardware/loadout configuration hook (validated non-negative; it does not itself emit a domain event, since it is not a player-facing maneuver command like `allocate_power`);
- the integration step formerly named `integrate_power_consumption` is renamed `integrate_energy_balance` and now nets `energy_generation_w` against `total_power_allocated_w()` each fixed step: consumption exceeding generation still depletes `stored_energy_j` exactly as before (clamped at zero), generation exceeding consumption now recharges it (clamped at `energy_capacity_j`), and an exact match is a genuine no-op;
- a new `EnergyRestored` domain event fires exactly once on the `0 -> >0` transition (mirroring `EnergyDepleted`'s `>0 -> 0` transition and `OverheatEnded`'s recovery pattern) and clears `is_energy_depleted`, which `refresh_capability_lockouts()` already combines with `is_overheated` so `can_scan`/`can_thrust` restore once no lockout cause remains active — no other code needed to change for the two causes to keep stacking correctly;
- `energy_generation_w` defaults to `0.0`: the canonical EV-0001 probe has no generation hardware equipped in this slice, so every pre-existing energy/thermal/overheat/depletion test continues to exercise exactly the same net-draw-only behavior as before, byte-for-byte unchanged;
- new CMake/CTest coverage exercises validation (negative generation rejected), the exact no-op case where generation precisely offsets allocated consumption, the recovery path (deplete stored energy under a net-negative balance, then drop consumption to zero so the surviving generation recharges it back above zero — firing `EnergyRestored` exactly once, restoring `can_scan`/`can_thrust`, and not re-firing on further steady net-positive steps), and the clamp-at-`energy_capacity_j` ceiling.

This does not yet touch `unreal/` for the same reason as the prior six slices, and does not equip the canonical probe with a nonzero `energy_generation_w` default (that remains a deliberate follow-up, likely bundled with future hardware-loadout or embodiment work) or begin option (b), the broader per-component operational/failure state beyond the two probe-wide lockouts.

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

`ScanCommand` (sequence item 4 below) and power allocation with its energy-consumption, energy-generation, and thermal-load effects (sequence item 5 below) are now implemented in `src/simulation/`, engine-independent and CTest-covered. Items 1–3 (the Unreal-side embodied probe runtime, transform-driving, and HUD read model) remain **not implemented**: they require compiling/running the Unreal project itself, which this scheduled run's environment cannot do (no Unreal Editor/UBT available to build or verify Unreal C++/Blueprint changes). Automation should not author unverifiable `unreal/Source/` changes; a run with Unreal build/verification capability should pick up items 1–3 next.

Recommended next sequence:

1. create a minimal runtime bootstrap in `unreal/` that instantiates exactly one probe presentation and exactly one `UProbeSimulationAdapter`;
2. drive the presented probe transform from the authoritative `src/simulation/` snapshot, with metres-to-centimetres conversion occurring only in the adapter/presentation boundary;
3. add a minimal inspect/HUD read model for mass, energy, temperature, storage, velocity, and simulation time;
4. ~~add the first real command path beyond movement: `ScanCommand` with validation plus `scan_started` / `scan_complete` events~~ — **done in `src/simulation/`**; still needs Blueprint/adapter exposure once item 1 exists;
5. begin power allocation and component-state mechanics — **in progress in `src/simulation/`**: `SimulationCore::allocate_power` validates and sets a per-subsystem (sensors/propulsion/computation/thermal) share of a total `power_capacity_w` budget and emits `PowerAllocationChanged`; allocated power draws down `stored_energy_j` over simulated time on the fixed-step path, net of a new configurable constant passive `energy_generation_w` supply (default `0.0`, not yet equipped on the canonical probe), clamped between zero and `energy_capacity_j`, emitting `EnergyDepleted` on the `>0 -> 0` transition and the new `EnergyRestored` on the `0 -> >0` recharge transition; allocated power also accumulates as waste heat into `temperature_k` over the same fixed-step path via a `thermal_capacity_j_per_k` field, combined with passive Newtonian cooling back toward `ambient_temperature_k` at a rate set by `passive_cooling_w_per_k`, so sustained allocated power drives `temperature_k` toward a finite equilibrium instead of climbing unbounded, and zero allocated power lets it drift back down to ambient; `temperature_k` crossing `max_operating_temperature_k` sets `is_overheated` and emits `OverheatStarted`, restoring it and emitting `OverheatEnded` once it drops back below the limit; `can_scan`/`can_thrust` are derived once per fixed step from **both** flags combined (`is_overheated || is_energy_depleted`) via `refresh_capability_lockouts()`, so the two lockout causes correctly stack instead of either response's own recovery wrongly restoring capabilities while the other cause is still active. Still pending within this item: equipping the canonical probe with a real nonzero `energy_generation_w` default, broader per-component operational/failure state beyond these two probe-wide lockouts (e.g. what happens once compute or propulsion themselves fail rather than the probe as a whole), and Blueprint/adapter exposure of all four commands (`ScanCommand`, `allocate_power`, `set_energy_generation_w`, and movement) once item 1 exists;
6. continue until the Phase 2 gate is demonstrably true: **simply existing as the probe is compelling.**

Option (a) named in this file's prior continuation note — an energy-recharge/generation mechanic that gives `is_energy_depleted` a real recovery path — is now **done**: `energy_generation_w` and `EnergyRestored` (above) implement it, defaulted off so the canonical probe's behavior is unchanged until a future slice equips it. The next highest-value engine-independent slice within item 5 is now option (b): starting the broader per-component operational/failure state (e.g. what happens once an individual subsystem — compute, propulsion, sensors — fails independently of the two current probe-wide lockouts). This is a larger, less-specified design surface than (a) was; scope it as its own deliberate slice rather than folding it into a single follow-up, and consider whether a smaller, self-contained warm-up slice (e.g. giving the canonical probe a real nonzero `energy_generation_w` default, which is a one-field, low-risk change now that the mechanic itself is implemented and tested) is worth doing first.

Also worth a dedicated near-term slice, independent of new mechanics: `.github/workflows/foundation.yml`'s `-DCMAKE_BUILD_TYPE=Release` step defines `NDEBUG`, which silently no-ops every `assert()` in `simulation_core_tests.cpp` unless the file itself defends against it. This slice added that defense (`#undef NDEBUG` before `#include <cassert>`) to `simulation_core_tests.cpp` and, while re-verifying with assertions now genuinely active, found and fixed one test whose expectations had gone stale (see `ERROR_RESOLUTION_LEDGER.md`, 2026-08-22). No other test in the suite was found to fail once assertions were made real, but that was only checked incidentally as a side effect of this slice's own verification, not via a systematic re-audit of every earlier slice's assertions — a dedicated follow-up should treat the entire suite's assertions as freshly unverified evidence (they were never actually checked by CI before now) and confirm each one deliberately, the same way `ERROR_RESOLUTION_LEDGER.md`'s existing per-key/per-branch coverage audits already did for the Python prototypes.

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
