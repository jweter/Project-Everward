# Everward Project Status

This file is the durable operational continuation record for human and scheduled development. It answers: **where are we now, what is verified, what remains open, and what should happen next?** Detailed historical implementation narratives belong in Git history, PRs, and `ERROR_RESOLUTION_LEDGER.md`.

## Current phase

**Phase 2 — One Probe: ACTIVE, first local integrated playtest pending.**

Phase 1 is complete and Unreal Engine 5.8 is the accepted production direction. Phase 2 now has a functioning engine-independent authoritative runtime plus an Unreal embodiment/control shell sufficient for the first serious local integration run.

The next gate is **evidence**, not another speculative subsystem: build and run the current One Probe slice locally in Unreal Engine 5.8, capture the structured first-run observation, then fix the highest-value defect or feel problem revealed by that run.

## Current `main` baseline

As of PR #100, `main` includes:

### Authoritative simulation

- engine-independent C++20 simulation under `src/simulation/`;
- deterministic fixed-step simulation time;
- canonical EV-0001 identity, generation, mass, position, velocity, energy, thermal state, storage, capability state, and subsystem power allocations;
- authoritative movement state;
- scan start/progress/complete/cancel lifecycle;
- power-budget validation across sensors, propulsion, computation, and thermal subsystems;
- stored-energy consumption plus canonical passive generation;
- waste-heat accumulation plus passive cooling;
- energy-depletion and overheat lockouts with recovery where physically supported;
- explicit subsystem operational/failure hooks and capability gating;
- production CMake/CTest coverage in GitHub Actions.

### Generation-1 automation

- engine-independent `ProbeRuntime` software-policy evaluator;
- one active Generation-1 policy with at most two simple rules;
- energy-fraction and temperature conditions;
- power-allocation actions routed through the same `SimulationCore::allocate_power()` mechanics used by manual control;
- policy execution gated by operational computation hardware and at least 25 W of computation power;
- policy install/clear/trigger/rejection events;
- temporary `gen1_basic_survival` integration preset.

### Unreal embodiment and control shell

- one production `AEverwardProbePawn` with one `UProbeSimulationAdapter`;
- adapter-driven presentation transform from authoritative simulation position;
- compact authoritative telemetry HUD;
- collapsed-by-default capability-driven systems panel;
- command acceptance/rejection feedback;
- shared adapter commands for velocity, scan start/cancel, power allocation, and Generation-1 policy interaction;
- deterministic runtime-generated Phase-2 player start and test environment;
- visible bootstrap scan target `phase2-test-target-001` about 50 m ahead;
- six spatial reference markers for movement/parallax evidence;
- temporary local lighting;
- third-person mouse orbit and wheel zoom;
- temporary three-axis 1 m/s velocity trims plus stop command.

### Evolution foundation

- adjacent-generation evolution generator;
- successor options derived from current capabilities and prerequisites rather than a fixed universal tech tree;
- bounded small-step generation changes;
- deterministic option generation;
- engineering tradeoff metadata and compute-dependent design breadth.

### Canonical Prime Probe production references

- Generation-1 Prime Probe A — Scientific Explorer is the canonical original design;
- canonical master, orthographic, dimensions, system-callout, deployment, manipulator/tool, and material reference sheets are versioned under `assets/reference/probe/gen1-prime/`;
- alternate B/C concepts remain exploratory references;
- production 3D blockout is still pending and does not block the first engineering playtest;
- canonical reference hashes and dimensions are now enforced by a standard-library
  validator in Foundation CI, establishing a safe parallel asset-production handoff.

## First-run integration gate

Use `docs/PHASE2_FIRST_RUN_PLAYTEST.md` and the structured observation template under `playtests/phase2/`.

The first run must objectively exercise:

- Unreal C++ build/module load;
- PIE launch and deterministic EV-0001 spawn;
- generated test environment;
- camera orbit/zoom;
- HUD/system discovery;
- X/Y/Z authoritative movement and stop;
- power allocation;
- scan completion and cancellation;
- policy install/clear;
- policy computation-power gate;
- manual and automation control of the same authoritative state.

Passing that protocol means the slice is **integrated and testable**. It does not by itself satisfy the Phase 2 roadmap gate that simply existing as the probe is compelling.

## Known non-blocking omissions

These are real future work, but they do not block the first local Phase-2 integration run:

- scan-result/discovery science payloads — Phase 3 concern after the scan interaction itself is proven;
- real world-object targeting — Phase 3;
- final thruster/attitude physics and control mapping;
- component-level health/wear/failure causes and repair mechanics;
- production save/load implementation;
- Prime Probe production 3D geometry/materials;
- final HUD styling;
- resource extraction, refining, fabrication, replication, and later-phase systems.

## Open product/architecture decisions

### Failure and repair semantics

`set_subsystem_operational()` remains a deterministic hook, not a final wear/damage model. Real failure causes, health representation, and repair semantics remain open. Do not invent a broad permanent-failure model before the industrial/recovery loop can support it.

### Final Generation-1 flight feel

The current 1 m/s velocity trims are temporary integration controls, not the final propulsion model. Direct playtest evidence should determine the next movement/attitude slice.

### Final programming interface

The two-rule Basic Survival policy proves the architectural loop only. Richer scripts, behavior editing, priorities, diagnostics, and compute-scaled automation remain later work. Manual and automated actions must continue to converge on shared authoritative commands.

## Authorized next work

Priority order after PR #101:

1. perform the local Unreal Engine 5.8 first-run protocol;
2. record the observation JSON and supporting screenshots/log excerpts as appropriate;
3. fix any build/crash or authoritative-state defect first;
4. then fix unusable controls or misleading HUD/automation behavior;
5. then tune Generation-1 embodiment/movement feel from evidence;
6. only after the first-run loop is stable, select the next Phase 2 gameplay slice or begin integrating the Prime Probe 3D blockout in parallel.

Do not jump ahead to Phase 3 astronomy, Phase 4 industry, replication, aliens, combat, megastructures, or broad procedural content before the One Probe embodiment is functioning and testable.

## Phase 2 production rules

- Simulation owns mechanical truth.
- Unreal consumes authoritative state and submits commands through the adapter boundary.
- `src/simulation/` remains buildable/testable without Unreal dependencies.
- Manual and automated controls must share authoritative mechanics.
- Canonical simulation units stay engine-independent; Unreal conversion happens at the presentation boundary.
- Deterministic headless execution remains required.
- Save data remains an explicit versioned schema rather than blind Unreal object serialization.
- Later descendants expose capabilities from installed hardware/software, not from a universal player ability list.
- Better computation may enable richer automation/design capability, but cannot bypass physical hardware, materials, manufacturing, or known science.

## Visual product constraint

Everward must not drift toward a primarily 2D, abstract-map, low-poly, deliberately quirky, or visually lightweight interpretation because it is easier to implement. The target remains cinematic, immersive, high-fidelity 3D scientific realism.

The temporary Phase-2 sphere, markers, labels, and light are engineering scaffolding only.

## Automation operating state

Scheduled development follows `AGENT_DEVELOPMENT_POLICY.md`:

1. inspect open PRs/CI first;
2. repair red work before starting new work;
3. merge only independently verified green and merge-ready work;
4. otherwise advance one highest-value authorized slice;
5. keep affected documentation current;
6. preserve accepted design/architecture decisions.

## Repository posture

- visibility: **Public by deliberate operational choice**;
- IP: **Proprietary, all rights reserved**;
- public visibility does not grant an open-source license;
- default branch: `main`;
- substantive development: branch + pull request, not direct-to-main.
