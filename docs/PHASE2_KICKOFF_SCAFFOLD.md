# Phase 2 Kickoff Scaffold

This document records the architecture boundary established by **ADR-0012** for Phase 2 — One Probe and the implementation status of that boundary after PR #68.

It elaborates `ARCHITECTURE.md`, `SIMULATION_PHILOSOPHY.md`, `SAVE_FORMAT.md`, ADR-0002, and ADR-0012. It is an architectural contract: later Phase 2 work may extend the implementation, but must preserve these ownership and dependency rules unless a later accepted ADR explicitly changes them.

## Current implementation status

**Foundation implemented.** PR #68 created the production Unreal project under `unreal/` and the first engine-independent C++20 authoritative simulation core under `src/simulation/`.

Implemented foundation:

- Unreal Engine 5.8 production project shell;
- C++20 simulation core buildable/testable independently of Unreal;
- canonical probe identity, position/velocity, mass, energy, temperature, storage, and basic capability state;
- deterministic fixed-step movement integration;
- domain-event delivery;
- `UProbeSimulationAdapter` as the single Unreal-side simulation caller;
- Blueprint-visible simulation tick, position, and velocity command access;
- `ScanCommand` (`SimulationCore::start_scan`) with validation (empty target, non-positive duration, capability gating, single concurrent scan) and `ScanStarted` / `ScanCompleted` domain events, integrated into the same fixed-step advance used for movement;
- `SimulationCore::allocate_power(PowerSubsystem, watts)` with per-subsystem power allocation (sensors, propulsion, computation, thermal) validated against a total `power_capacity_w` budget, rejecting negative requests and any combined allocation that would exceed capacity, and emitting a `PowerAllocationChanged` domain event on success;
- allocated power draws down `stored_energy_j` over simulated time on the same fixed-step path as movement and scan progress, clamped at zero, emitting an `EnergyDepleted` domain event on the transition from having stored energy to having none;
- allocated power also accumulates as waste heat into `temperature_k` over simulated time on the same fixed-step path, using a `thermal_capacity_j_per_k` probe field, combined with passive Newtonian cooling back toward a new `ambient_temperature_k` field at a rate set by a new `passive_cooling_w_per_k` field, solved in closed form so `temperature_k` approaches a finite equilibrium under sustained power rather than climbing unbounded, and drifts back toward ambient once power is deallocated;
- `temperature_k` crossing a new `max_operating_temperature_k` field triggers an overheat lockout: a new `is_overheated` flag is set and an `OverheatStarted` domain event fires; dropping back below the limit clears the flag and fires `OverheatEnded`;
- `stored_energy_j` reaching zero similarly sets a new `is_energy_depleted` flag, reusing the existing `EnergyDepleted` event as its transition marker;
- a new `energy_generation_w` probe field models a constant passive power supply (RTG-style, not distance-dependent solar) that offsets allocated consumption on the same fixed-step path; net generation now recharges `stored_energy_j` (clamped at `energy_capacity_j`) and clears `is_energy_depleted` via a new `EnergyRestored` domain event on the zero-to-positive transition, closing the previously one-way depletion lockout. `energy_generation_w` defaults to `0.0` — the canonical probe has no generation hardware equipped yet — and is configured via `SimulationCore::set_energy_generation_w(watts)`;
- `can_scan`/`can_thrust` are derived once per fixed step from both lockout flags combined (`is_overheated || is_energy_depleted`), so the two causes correctly stack rather than one's recovery wrongly restoring capabilities while the other remains active;
- production-core CMake/CTest coverage in GitHub Actions.

Not yet implemented at this reconciliation point:

- visible embodied probe runtime scene driven from the authoritative snapshot;
- complete component model for sensors, computation, propulsion, and thermal behavior beyond the power-allocation budget, energy-consumption effect, energy-generation/recharge effect, thermal-load/passive-cooling effect, and the two probe-wide lockouts (overheat, energy depletion) above (e.g. equipping the canonical probe with a real nonzero `energy_generation_w` default, or per-component operational state/failure modes beyond these two probe-wide lockouts);
- software policy state and alter-policy interaction;
- `ScanCommand`, `allocate_power`, and `set_energy_generation_w` exposure through `UProbeSimulationAdapter`/Blueprint (all three exist in `src/simulation/` only; no Unreal-side caller yet);
- scan results/discovery payloads (current `ScanCommand` proves the start/validate/complete lifecycle and timing, not scan outcome content);
- complete inspect/alter-policy interaction paths;
- production save/load implementation;
- stronger-hardware production validation capture.

`PROJECT_STATUS.md` is the authoritative continuation record for which of these is next.

## 1. Repository layout

### 1.1 Unreal project root: `unreal/`

The production Unreal project lives at the top-level `unreal/` directory, sibling to `src/`, `prototypes/`, `tests/`, `tools/`, `assets/`, and `docs/`.

```text
unreal/
├── Everward.uproject
├── Config/               # as production configuration is added
├── Content/              # production content as it is added
├── Source/
│   └── Everward/
└── README.md
```

`prototypes/rendering-benchmark/unreal/` remains frozen historical Phase 1 evidence. It is not the production game project.

### 1.2 Simulation core: `src/simulation/`

The authoritative simulation core is production C++20 code under `src/simulation/`, compiled independently with CMake/CTest and consumed by the Unreal adapter without introducing Unreal dependencies into simulation truth.

The longer-term logical module decomposition remains the architecture target:

```text
src/simulation/
├── time/
├── coordinates/
├── probe/
├── environment/
├── software_policy/
└── boundary/
```

The initial PR #68 implementation is intentionally smaller than that final decomposition. New Phase 2 mechanics should be separated into these responsibilities as they become substantive rather than prematurely creating empty modules.

`astronomy`, `industry`, `research`, `engineering`, `messages`, `lineage`, `autonomous_agency`, `difficulty`, and `history` remain later-phase concerns unless required by an earlier accepted gate.

### 1.3 Promotion rules

Phase 1 Python prototypes remain technical evidence, not shipping runtime code. Production promotion means reimplementing proven behavior in production-appropriate code while preserving its tested invariants.

Promoted simulation code must:

1. remain buildable and testable with zero Unreal header/type dependency;
2. preserve deterministic behavior;
3. reproduce relevant golden fixtures where a prototype fixture actually applies;
4. leave historical prototype evidence intact rather than rewriting it into production code.

### 1.4 Ownership boundary

- Mechanical truth does not live under `unreal/Source/`.
- Unreal code owns presentation, input translation, and the adapter boundary only.
- No Unreal Actor, Component, Blueprint, or widget may call `src/simulation/` directly except through the designated adapter type.
- No Unreal-specific type or unit representation belongs in authoritative simulation state.

## 2. Authoritative-state/presentation boundary

### 2.1 Principle

"Unreal reads authoritative simulation state without owning it" means:

- Unreal does not independently decide scan results, trajectories, energy outcomes, thermal outcomes, component capabilities, or policy effects.
- Unreal reads simulation-produced state/events and submits commands for the simulation to validate and resolve.
- Values crossing the boundary are plain data, not mutable references into simulation-owned objects.

### 2.2 State channel — simulation → presentation

The complete Phase 2 target snapshot contains:

| Field group | Target contents | Current status |
|---|---|---|
| Identity | probe ID, design ID, lineage ID, generation | foundation present; extend as Phase 2 requires |
| Position / velocity | canonical simulation-space position and velocity | implemented |
| Mass | total and component mass | initial total state present; component breakdown pending |
| Energy | generation, storage, charge, subsystem consumption | initial energy state present; per-subsystem power allocation (sensors, propulsion, computation, thermal) validated against total capacity is present; allocated power draws down stored energy over simulated time, net of a configurable constant passive `energy_generation_w` supply (`EnergyDepleted` on depletion, `EnergyRestored` on recharge back above zero); depletion sets `is_energy_depleted` and locks out `can_scan`/`can_thrust` alongside the overheat lockout, restored on recharge; `energy_generation_w` defaults to `0.0` on the canonical probe (no generation hardware equipped yet), so a real default generation source and any distance-dependent solar model remain pending |
| Thermal | thermally relevant component temperatures and limits | initial temperature state present; allocated power (`total_power_allocated_w()`) accumulates as waste heat into `temperature_k` over simulated time on the same fixed-step path as movement/scan/energy, combined with passive Newtonian cooling back toward `ambient_temperature_k` so sustained load approaches a finite equilibrium rather than climbing unbounded; `temperature_k` crossing `max_operating_temperature_k` now clears `can_scan`/`can_thrust` and fires `OverheatStarted`/`OverheatEnded`; per-component thermal limits beyond this probe-wide response remain pending |
| Storage | resource inventory and capacity | initial storage state present; inventory mechanics pending |
| Sensors | sensor components, targets, progress, scan-result references | scan lifecycle state (`is_scanning`, active target, remaining duration) present; sensor components and scan-result content pending |
| Computation | compute components, utilization, policy assignment | pending |
| Propulsion | propulsion components, thrust/budget, maneuver state | basic velocity command path present; full propulsion model pending |
| Component capabilities | operational state and capability flags | basic capability state present; component model pending |
| Software state | active policy and autonomy parameters | pending |

Domain events carry discrete simulation happenings so presentation can react without becoming authoritative.

### 2.3 Command channel — presentation → simulation

The Phase 2 interaction contract remains:

| Interaction | Boundary behavior | Status |
|---|---|---|
| Observe | read current authoritative snapshot/events | foundation present |
| Inspect | read component/system state | minimal state available; HUD/read model pending |
| Scan | validated scan command and lifecycle events | `src/simulation/` command/lifecycle present (`start_scan`, `ScanStarted`/`ScanCompleted`); Unreal-side command path and scan-result content pending |
| Move | validated movement/trajectory request | initial velocity-command path present |
| Manage power | validated allocation request | `src/simulation/` command/validation present (`allocate_power`, `PowerAllocationChanged`); Unreal-side command path and component-level power effects pending |
| Alter policy | validated software-policy request | pending |

Every mutating command follows:

> Command → Validation → State transition → Domain event

Presentation may surface rejection but must not bypass it.

### 2.4 Cadence and simulation time

The Unreal adapter is the sole Unreal-side driver/caller of the simulation core. It uses a fixed-step accumulator rather than allowing raw variable render-frame timing to directly author mechanical state.

The adapter:

1. accumulates rendered frame delta;
2. drains whole fixed simulation steps;
3. advances authoritative simulation for each whole step;
4. reads the resulting snapshot/events;
5. converts canonical values for presentation;
6. submits player commands back to the simulation core.

This fixed-step rule is implemented in PR #68 and supersedes the documentation-only repair attempted in PR #67.

### 2.5 Units

Canonical simulation values use engine-independent units such as metres, seconds/ticks, kelvin, joules/watts, and kilograms. Unreal world-space presentation uses centimetres.

Conversion occurs at the adapter/presentation boundary. It must not leak into `src/simulation/` or be performed multiple times for the same value.

### 2.6 Persistence

Canonical campaign saves remain simulation-owned, explicitly versioned data per ADR-0004 and `SAVE_FORMAT.md`. Unreal object serialization is not the canonical campaign-save architecture.

Presentation-only preferences may eventually be stored separately, but must not become authoritative simulation state.

## 3. Residual rendering risk

Phase 1 hardware evidence on Intel Iris Xe at 2560×1440 recorded a GPU frame time of 61.63 ms and internal rendering around 60.3% resolution. That missed the 60 FPS target on that hardware and demonstrated meaningful GPU risk.

This remains a tracked production risk, not a blocker on Phase 2:

- keep the first One Probe runtime scene representative but deliberately minimal;
- make scalability/resolution choices explicit rather than hidden;
- do not use early Phase 2 as an excuse to lower the visual product target;
- validate the production One Probe slice on stronger discrete-GPU hardware when available.

The final product target remains cinematic, immersive, high-fidelity 3D scientific realism.

## 4. Current Phase 2 implementation boundary

The kickoff foundation is implemented, but the Phase 2 gate is **not** satisfied merely because the project and simulation core compile.

Work remains inside Phase 2 until the player can meaningfully inhabit one probe and use the complete required interaction set: observe, scan, move, inspect systems, manage power, and alter basic software policies.

Do not use this scaffold to justify premature Phase 3+ implementation. `ROADMAP.md` defines phase order and `PROJECT_STATUS.md` defines the exact continuation point.

## See also

- `PROJECT_STATUS.md` — current operational continuation point.
- `ROADMAP.md` — authoritative phase sequence and gates.
- `ARCHITECTURE.md` — logical layers and hard boundaries.
- `SIMULATION_PHILOSOPHY.md` — simulation truth and determinism.
- `SAVE_FORMAT.md` — persistence contract.
- `DECISION_LOG.md` ADR-0002 and ADR-0012 — accepted ownership/boundary decisions.
- `unreal/README.md` — production Unreal project boundary notes.
