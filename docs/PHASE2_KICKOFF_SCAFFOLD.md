# Phase 2 Kickoff Scaffold

This document is the concrete design referenced by **ADR-0012** in `DECISION_LOG.md`. It defines where the initial Unreal project will live, how simulation-core logic is promoted out of `prototypes/` into `src/`, and precisely what "Unreal reads authoritative simulation state without owning it" means as a boundary contract for the Phase 2 — One Probe embodiment.

It elaborates `ARCHITECTURE.md`'s logical layers, `SIMULATION_PHILOSOPHY.md`'s "the simulation owns truth" rule, `SAVE_FORMAT.md`, and ADR-0002, applied specifically to standing up the first probe.

**Scope note:** this document is a design boundary, not an implementation. See "Explicitly out of scope" (§5) before assuming any of this exists yet.

## 1. Repository layout

### 1.1 Unreal project root: `unreal/`

The production Unreal project is a new top-level directory, a sibling of `src/`, `prototypes/`, `tests/`, `tools/`, `assets/`, and `docs/` — not nested inside either of those:

```text
unreal/
├── Everward.uproject
├── Config/
├── Content/
├── Source/
│   └── Everward/          # the only Unreal-side module allowed to touch src/
└── README.md               # boundary rules for this directory, mirrored
                             # from prototypes/rendering-benchmark/unreal/README.md
```

This mirrors the directory shape already used by the disposable Phase 1 benchmark project (`prototypes/rendering-benchmark/unreal/`, project name `EverwardBenchmark`), and matches the Unreal-generated-state ignore rules already present in the root `.gitignore` (`Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `.vs/`), which are not scoped to `prototypes/` and were clearly anticipating a top-level Unreal tree.

`prototypes/rendering-benchmark/unreal/` is **not** promoted or reused as the production project. It remains frozen as Phase 1 historical evidence (its `EverwardBenchmark` module, static handoff-file playback, and one-shot capture instrumentation were built to answer a benchmark question, not to be the production game). The production `unreal/Source/Everward/` module starts clean and reuses only patterns proven there — see §2.7.

### 1.2 Simulation-core promotion: `src/`

`src/` gains an internal module layout that mirrors `ARCHITECTURE.md`'s "Candidate core modules" list, scoped for the kickoff slice to only the modules the One Probe embodiment actually needs:

```text
src/
└── simulation/
    ├── time/            # promoted from prototypes/simulation-clock
    ├── coordinates/      # promoted from prototypes/coordinate-scale
    ├── probe/             # new: mass, components, capabilities
    ├── environment/       # new: energy, thermal, storage
    ├── software_policy/   # new: policy state, autonomy mode
    └── boundary/          # the state/event/command contract types in §2,
                            # owned by simulation, importable by an adapter,
                            # containing zero Unreal types or headers
```

`astronomy`, `industry`, `research`, `engineering`, `messages`, `lineage`, `autonomous_agency`, `difficulty`, and `history` remain out of scope for the kickoff slice; they promote later, as their owning phases (3+) authorize them.

**Promotion is a port, not a copy-paste.** The Python prototypes proved algorithms and contracts, not shipping code — `docs/TECHNOLOGY_DECISIONS.md` TD-005 keeps Python out of the shipping runtime. Promoting a prototype into `src/simulation/` means:

1. re-implementing its behavior in whatever production-appropriate language/toolchain a module actually needs (this document does not select that language — it is a separate, still-open decision),
2. keeping the promoted code buildable and testable with **zero dependency on Unreal headers, macros, or types** (no `UObject`, no `UCLASS`, no Unreal containers) — it must build and run headless, consistent with ADR-0002/TD-003 and `SIMULATION_PHILOSOPHY.md`'s "headless simulation is a first-class capability",
3. proving parity against the existing prototype using its golden fixtures as a cross-implementation oracle where they exist — e.g. `prototypes/headless-simulation/golden_runs.json` for the clock/event-scheduler behavior being promoted from `prototypes/simulation-clock`, and `prototypes/procedural-system/golden_seeds.json` if/when procedural generation promotes. A promoted module is not done until it reproduces the relevant golden fixture's outputs bit-for-bit under the same seed/inputs.

`prototypes/` itself is untouched by promotion — it keeps its own tests and README as Phase 1 evidence; nothing under it is deleted or rewritten as part of standing up `src/`.

### 1.3 Where things do not go

- No simulation-core logic (mass/energy/thermal/propulsion resolution, command validation, state transitions) lives under `unreal/Source/`. That directory holds presentation, input translation, and the adapter described in §2.7 only.
- No Unreal-specific data (UE units, `UObject` references, Actor/Component state) lives under `src/`. `src/simulation/boundary/` types are plain data usable by any presentation layer, not just Unreal, in principle.

## 2. Authoritative-state/presentation boundary

### 2.1 Principle

"Unreal reads authoritative simulation state without owning it" means, concretely:

- Unreal never computes a mechanical outcome (a scan result, a trajectory change, a power allocation, a policy effect) itself. It only ever (a) reads what the simulation core has already decided, and (b) submits a request for the simulation core to decide something.
- Exactly one Unreal-side object type is permitted to call into `src/simulation/` at all (§2.7). No Actor, Component, Blueprint, or UI widget talks to simulation state directly.
- Every value that crosses the boundary is plain data (the types in `src/simulation/boundary/`), not a live reference into simulation-owned objects. Unreal cannot accidentally mutate simulation truth by holding a pointer/reference into it.

### 2.2 State channel — simulation → presentation

Once per relevant simulation update, the simulation core produces a **Probe State Snapshot** covering every field named in `PROJECT_STATUS.md`'s Phase 2 continuation point:

| Field group | Contents | Notes |
|---|---|---|
| Identity | probe ID, design ID, lineage ID, generation | stable IDs per `ARCHITECTURE.md` entity-identity rule |
| Position / velocity | canonical simulation-space position and velocity | canonical units (metres, m/s); coordinate representation follows whatever `prototypes/coordinate-scale` proves out, converted only at the boundary (§2.6) |
| Mass | total mass, per-component mass breakdown | informational; Unreal never resolves physics from it |
| Energy | generation rate, storage capacity, current charge, per-subsystem consumption | |
| Thermal | temperature per thermally-relevant component, operating limits | |
| Storage | resource inventory (material → quantity), capacity, current fill | |
| Sensors | installed sensor components and their specs, current scan target, scan progress, latest available scan-result reference | |
| Computation | installed compute components, compute budget/utilization, active policy assignment | |
| Propulsion | installed propulsion components, max thrust, available delta-v/fuel budget, current maneuver state | |
| Component capabilities | per-component operational status (nominal / degraded / offline), capability flags (can-scan, can-thrust, can-fabricate, ...) | |
| Software state | active policy ID and parameters, autonomy mode | deterministic/rule-based only, per ADR-0007 |

Alongside the snapshot, the simulation core emits **domain events** for discrete happenings since the last read (`scan_started`, `scan_complete`, `component_state_changed`, `policy_changed`, `maneuver_started`, `maneuver_complete`, ...), so the adapter can trigger one-shot VFX/audio/HUD callouts without diffing snapshots itself.

### 2.3 Command channel — presentation → simulation

The roadmap's six named interactions map onto this channel as follows:

| Interaction | Boundary shape | Mutates truth? |
|---|---|---|
| Observe | Local read of the current Probe State Snapshot (and cached scan results already delivered) | No — pure read |
| Inspect | Local read of a specific component's current state from the snapshot | No — pure read |
| Scan | `ScanCommand(target_id, sensor_component_id)` | Yes — validated, produces `scan_started`/`scan_complete` events and, on success, new discovery/scan-result state |
| Move | `SetTrajectoryCommand(destination \| burn_vector, propulsion_component_id)` | Yes — validated, produces `maneuver_started`/`maneuver_complete` events and updated position/velocity |
| Manage power | `SetPowerAllocationCommand(allocations: component_id → fraction)` | Yes — validated, produces `component_state_changed` events |
| Alter policy | `SetPolicyCommand(policy_id, parameters)` | Yes — validated, produces `policy_changed` events |

Every command follows `ARCHITECTURE.md`'s existing pipeline: Command → Validation → State transition → Domain event. The simulation core may reject a command (insufficient power, no line of sight, propulsion offline, ...); the adapter surfaces rejection as a read-only outcome, it does not retry or work around it locally.

### 2.4 Cadence

- **State + events, simulation → presentation:** pulled once per Unreal engine tick by the adapter (§2.7), but only reflect what actually changed on the simulation side — the simulation core advances on its own event-driven clock (`SIMULATION_PHILOSOPHY.md`), not on rendered frame rate. Local-fidelity probe state updates at high frequency near the player, consistent with the "local fidelity, distant abstraction" rule; there is no distant/aggregated tier to worry about yet in the One Probe slice.
- **Commands, presentation → simulation:** pushed on-demand, whenever player input produces one of the six interactions above. Not tied to a fixed cadence.
- **Persistence:** out of band from both channels — see §2.6.

### 2.5 Units and the conversion boundary

Canonical simulation values stay in the units already assumed by `ARCHITECTURE.md` and `SAVE_FORMAT.md` (metres, seconds/ticks, kelvin, joules/watts, kilograms). Unreal world space is centimetres. Exactly one place performs unit conversion: the adapter (§2.7), on the way out to Unreal and on the way in from player input — the same rule Prototype C already established (`prototypes/rendering-benchmark/unreal/README.md`: "the adapter performs an explicit 100× unit conversion at the presentation boundary. The conversion must not leak back into simulation truth."). No conversion happens inside `src/simulation/`, and no conversion happens more than once per value.

### 2.6 Persistence boundary

Unreal does not own or independently persist probe mechanical state. Saves remain a simulation-core-owned versioned schema per ADR-0004 and `SAVE_FORMAT.md`; if Unreal ever needs to persist anything of its own (camera preferences, HUD layout, local settings), that is a separate, clearly-labeled presentation-preferences store, never mixed into the campaign save schema described in `SAVE_FORMAT.md`.

### 2.7 The adapter

A single Unreal-side type, informally `ProbeSimulationAdapter`, is the only code in `unreal/Source/Everward/` permitted to call into `src/simulation/`. Each engine tick it:

1. drives simulation time forward for this tick (§2.8) — without this step the simulation clock never advances and scheduled `scan_complete`/`maneuver_complete` events never fire, since `SimulationClock` is purely passive (`prototypes/simulation-clock/clock.py`),
2. pulls the latest Probe State Snapshot and any new domain events from `src/simulation/boundary/`, reflecting whatever step 1 just advanced,
3. performs the unit/type conversion in §2.6 and applies the result to Actors/Components/HUD read models,
4. translates player input into one of the Command types in §2.3 and submits it to the simulation core's command queue,
5. runs no independent movement, energy, thermal, or capability logic of its own.

This is the same shape as Prototype C's `ABenchmarkAdapter` / `UBenchmarkCaptureSessionComponent` (read a boundary contract, convert units, drive presentation, never author scene truth locally), generalized from a one-shot static handoff file to a live bidirectional state/command channel.

### 2.8 Driving simulation time forward

The promoted `src/simulation/time/` module carries forward `prototypes/simulation-clock/clock.py`'s `SimulationClock`, which is **passive**: its own tick counter only changes inside `advance_to(target_tick)` / `advance_by(delta_ticks)` / `advance_wall_ticks(wall_ticks)`. Nothing else in the simulation core calls those methods on its own — left undriven, `tick` never moves, `run_until_idle`/`advance_*` never fire the events already sitting in the queue, and `scan_complete`/`maneuver_complete` (§2.2) never occur no matter how much wall-clock time passes. The boundary contract therefore has to say who calls the advance method, with what argument, and when, relative to the read in step 2 above.

**Who calls it, and why that is not a second caller.** `ProbeSimulationAdapter` (§2.7) does — the same and only object type permitted to call into `src/simulation/`. Advancing time is classified as command submission, not a distinct responsibility competing with the boundary rule in §2.1 ("exactly one Unreal-side object type is permitted to call into `src/simulation/`"): the adapter calling `advance_wall_ticks()` is exactly as much "the adapter telling the simulation core to do something" as the adapter calling `ScanCommand` or `SetTrajectoryCommand` is. There is no separately-owned simulation runner and no second caller; time-advance is simply the one command type that runs unconditionally every tick instead of only when the player acts.

**Which method, and what it takes.** The adapter calls `advance_wall_ticks(wall_ticks: int)`, not `advance_to()`. `advance_wall_ticks` is the method the prototype built specifically for a real-time driver: it takes an integer count of elapsed **wall-clock ticks** (the same 1-tick-per-microsecond unit as everything else in the module — `TICKS_PER_SECOND = 1_000_000`), applies `time_scale` using exact `Fraction` arithmetic, and carries the fractional remainder forward across calls so that "different wall-step chunking produces the same simulation time" (its own docstring). `advance_to()` stays reserved for callers that already know the target tick outright — load-a-save, deterministic tests, tools — not the per-tick live-drive path, which only ever knows elapsed time, not an absolute tick. Concretely, the adapter converts Unreal's tick delta to this unit once, at the same conversion point already established in §2.5 (the adapter is the only unit-conversion site), then passes the integer result to `advance_wall_ticks()`.

**Reconciling a passive tick against Unreal's variable render framerate.** Feeding `advance_wall_ticks()` Unreal's raw per-frame `DeltaTime` directly would make simulated time a function of hitches, vsync behavior, and per-machine frame pacing — none of which are in `SIMULATION_PHILOSOPHY.md`'s determinism list ("given the same universe seed, generation algorithm version, initial state, player commands, autonomous-agent decisions, and deterministic random streams, Everward should reproduce the same mechanical results"). Raw render delta-time is exactly the kind of hidden, unrecorded input that rule is meant to exclude. The adapter instead runs a fixed-timestep accumulator decoupled from the variable render tick, the standard mechanism for reconciling a deterministic simulation clock with a variable-framerate renderer:

- each engine tick, the adapter adds Unreal's `DeltaTime` (converted to the tick unit above) into a real-valued accumulator it owns;
- while the accumulator holds at least one fixed step's worth of ticks (an adapter-owned constant, e.g. `kFixedSimStepTicks`, chosen independently of render framerate), it drains one step at a time, calling `advance_wall_ticks(kFixedSimStepTicks)` once per step — zero, one, or several calls in a given engine tick depending on how much real time that tick covered;
- any leftover fractional accumulator value carries over to the next tick, exactly mirroring how `advance_wall_ticks()` already carries its own `_scale_remainder` forward internally.

This makes "how much simulated time advanced" a function of a whole number of fixed steps rather than of raw hardware timing jitter. Live play across different hardware legitimately advances different amounts of *real* time — that is expected of any real-time game and is not a determinism violation — but the *simulated* result for a given number of elapsed fixed steps, the same player commands, and the same autonomous-agent decisions is reproducible, and a recorded run can be replayed deterministically by replaying the recorded fixed-step count rather than by resampling live hardware timing.

## 3. Residual rendering risk guidance

`PROJECT_STATUS.md` logs real residual risk from the Phase 1 hardware capture (Intel Iris Xe, 2560×1440): GPU frame time missed the 60 FPS / ~16.7 ms target (61.63 ms, strongly GPU-bound), and the output was internally upscaled from ~60.3% render resolution rather than rendered natively. This document does not relitigate that finding (PR #63 already accepted it as tracked production-quality risk, not an ADR-0001 blocker); it only says how the kickoff scaffold should account for it:

- The kickoff scene for the One Probe slice should stay deliberately minimal/representative (the probe, its immediate surroundings, no unrelated showcase content) so early Phase 2 performance signal is not confounded by visual load unrelated to the boundary being proven. Phase 2's own gate is "simply existing as the probe is compelling," not Phase 22 visual-production quality.
- Any scalability/resolution settings the scaffold introduces (dynamic resolution, upscaling, quality presets) must be explicit and visible in `Config/`, not a hidden default — so a future capture can tell whether a result reflects native rendering or upscaling, which the Phase 1 capture could not cleanly distinguish.
- Heavier visual ambition (dense volumetrics, large particle fields, the wallpaper-quality standard from `VISUAL_DIRECTION.md`) stays explicitly deferred past the kickoff scaffold rather than assumed as a kickoff requirement.
- A stronger-hardware (discrete GPU) validation capture of the One Probe slice remains a tracked follow-up once that hardware is available, consistent with `PROJECT_STATUS.md`'s existing note that such a capture "remains useful follow-up evidence" — not a blocker on starting the kickoff implementation work this document authorizes designing.

## 4. Explicitly out of scope

This document, and the ADR that references it, do **not**:

- add any Unreal project files (`.uproject`, `Config/`, `Source/`, `Content/`) under a new `unreal/` directory or anywhere else,
- add any C++ or Blueprint code,
- add or modify any `src/` implementation code (the `src/simulation/` layout in §1.2 is a target shape, not code that exists after this change — `src/README.md`'s placeholder status is unchanged by this document),
- select the production implementation language for `src/simulation/`,
- change anything under `prototypes/`,
- resolve the still-open coordinate-representation question that `prototypes/coordinate-scale` is scoped to answer,
- perform or schedule the stronger-hardware capture referenced in §3.

Those are the next authorized slice(s), gated on this design (ADR-0012) actually being accepted through normal review — i.e. this pull request merging to `main` — not on anything further this document does on its own.

## See also

- `ARCHITECTURE.md` — logical layers, hard boundaries, candidate core modules, entity identity.
- `SIMULATION_PHILOSOPHY.md` — simulation owns truth, headless-first, local fidelity.
- `SAVE_FORMAT.md` — probe state fields, versioned schema requirement.
- `ROADMAP.md` — Phase 2 — One Probe scope and gate.
- `PROJECT_STATUS.md` — Phase 2 continuation point and residual rendering risk.
- `prototypes/rendering-benchmark/unreal/README.md` — the adapter/unit-conversion pattern this design generalizes.
- `DECISION_LOG.md` ADR-0012 — the decision record this document supports.
