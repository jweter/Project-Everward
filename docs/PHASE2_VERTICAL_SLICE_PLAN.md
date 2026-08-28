# Phase 2–4 Vertical Slice Execution Plan

This document is the detailed execution plan beneath `ROADMAP.md` for turning Everward from an integrated engineering prototype into a compelling embodied probe game. It exists so human and automated development can choose the next **small, complete, player-visible slice** without rediscovering priorities from conversation history.

The master roadmap remains authoritative for phase boundaries. This plan refines the work inside late Phase 2, Phase 3, and the beginning of Phase 4.

## Development rule

Prefer one small vertical slice at a time:

1. inspect current `main`, open PRs, CI, and latest Product Reality evidence;
2. choose the smallest missing capability that materially improves what the player can see, understand, or do;
3. implement authoritative mechanics first and presentation through the existing Unreal adapter boundary;
4. add deterministic/source-contract regression coverage;
5. require green CI;
6. run the exact passed build locally in Unreal;
7. record Product Reality findings;
8. merge only when the slice works in the packaged/local test, **unless it qualifies for the parallel-safe lane below**;
9. choose the next slice from this plan.

### Parallel-safe lane while Product Reality is pending

A pending local Unreal/Product Reality check is a **completion and release gate**, not automatically a repository-wide development stop. Independent, reversible work may continue and may merge before the pending local check when all of the following are true:

- the new work does not assume that the unverified behavior is correct;
- it does not change, hide, compensate for, or weaken the behavior currently awaiting Product Reality;
- it reads/writes authoritative state only through existing simulation/adapter boundaries;
- deterministic or source-contract CI protects the new behavior as far as portable CI can reasonably verify it;
- the PR and project status explicitly say **implemented, Product Reality pending** rather than claiming the slice or phase complete;
- the new behavior is added to the next local Unreal test script so its visible/player-facing result is still checked;
- any Product Reality failure in the earlier slice still outranks later roadmap work and is repaired before phase/release advancement.

Examples of parallel-safe work include a HUD view over already-authoritative telemetry, deterministic data-model work that does not depend on the pending visual result, or reversible presentation around a stable adapter contract. Examples that do **not** qualify include tuning controls whose feel is still unverified, building mechanics on an unverified collision/physics result, or marking a phase/release gate complete without local evidence.

This lane preserves Product Reality as the definition-of-done authority while allowing the repository to keep advancing when the pending check is genuinely independent.

Avoid large speculative feature bundles. Infrastructure work is justified when it directly enables the next visible slice.

## Environment target

Everward is both a **space game and a planetary-environment game**. The probe must eventually operate across a continuous physical context:

`interstellar space -> star-system space -> orbital space -> asteroid/moon proximity -> planetary approach -> atmosphere when physically possible -> surface/near-surface operations`

Do not architect the game around an infinite flat ground plane. The current Phase-2 terrain scene is test scaffolding only.

Different environments may require different physical effects, but they must share the same authoritative state and command model. Deep space should emphasize inertia and relative motion; planetary contexts may add gravity, altitude, local horizon, atmosphere, terrain contact, and orbital dynamics as the simulation grows.

## Slice sequence

### Slice 1 — Scan payoff and control legibility

**Status:** in progress via Product Reality work around scan feedback and flight HUD.

Player-visible result:

- scan visibly progresses and completes;
- completion produces a persistent discovery result;
- cancellation produces no false discovery;
- velocity/speed/direction are visible while moving;
- Spacebar is a global emergency brake regardless of selected subsystem.

### Slice 2 — Persistent subsystem consequences

**Status:** parallel-safe implementation may proceed while Slice 1 Product Reality remains pending. The compact at-a-glance subsystem power/operational-state view is the first independent sub-slice; Slice 2 is not complete until its local Unreal evidence is recorded.

Player-visible result:

- all installed subsystems show live power and operational state at a glance;
- power allocation changes have visible consequences where mechanics already exist;
- rejected/limited actions explain why;
- automation actions show what changed and why.

### Slice 3 — Collision and contact foundation

This is a near-term requirement. The probe must not ghost through terrain, planets, asteroids, structures, or other physical bodies.

Authoritative requirements:

- probe has a defined physical collision envelope separate from decorative art;
- physical world objects expose collision/contact surfaces;
- contact reports collision point, surface normal, and relative velocity;
- motion is blocked/resolved rather than passing through solid matter;
- impact telemetry is deterministic enough for headless tests;
- Unreal presents collision but does not become the sole owner of damage truth.

Player-visible result:

- flying into a solid object causes contact instead of clipping through it;
- HUD communicates an impact/contact event;
- low-speed contact and dangerous impact are distinguishable.

Do **not** add arbitrary hit points as a shortcut. Preserve the information required for later physically grounded damage.

### Slice 4 — Impact severity and damage foundation

Build on contact data:

`probe body -> collision -> relative velocity -> effective mass/impact energy -> affected component -> consequence`

Initial scope:

- calculate impact severity from physically meaningful inputs;
- classify harmless/light/damaging/severe impacts;
- permit deterministic subsystem impairment;
- expose which component/system was affected and why;
- keep repair semantics simple until the industrial loop can support real recovery.

### Slice 5 — Prime Generation-1 probe body blockout

Replace the temporary sphere/hemisphere with a recognizable blockout based on the canonical **Probe A / Scientific Explorer / Prime Generation-1** reference package.

Minimum visible systems:

- main structural spine/body;
- computation/core region;
- power/reactor region;
- propulsion assembly;
- maneuvering hardware;
- sensor/telescope hardware;
- thermal radiators;
- manipulator mounting points.

Requirements:

- approximately correct scale and silhouette;
- collision envelope corresponds meaningfully to the physical body;
- camera framing/orbit works with the actual shape;
- design remains modular enough for later component damage/replacement;
- production art is not required yet, but the player must unmistakably inhabit a spacecraft rather than a placeholder primitive.

### Slice 6 — Articulated manipulator arms

**Status:** two sub-slices implemented via the parallel-safe lane: (1) authoritative deploy/stow/tool mechanics, adapter command/telemetry surface, minimal input, compact HUD status — see `PHASE2_MANIPULATOR_ARM_FOUNDATION_TEST.md`; (2) joint articulation input and a dedicated manipulator HUD page over those same authoritative mechanics — see `PHASE2_MANIPULATOR_JOINT_ARTICULATION_TEST.md`. Visible arm geometry/animation remains the next sub-slice; Slice 6 is not complete until it lands and local Unreal evidence is recorded for the whole slice.

Add at least two real articulated manipulators to the Prime probe.

Initial mechanical chain:

`shoulder -> upper arm -> elbow -> forearm -> wrist -> tool interface`

Player-visible result:

- deploy/stow arms;
- move shoulder/elbow/wrist through constrained ranges;
- visible tool attachment interface;
- collision does not allow impossible penetration through the probe body.

### Slice 7 — Object selection and physical interaction

Turn scanning and manipulators into one loop:

`detect -> select -> approach -> scan -> reach -> grasp -> move -> release`

Minimum object interactions:

- select a nearby physical target;
- display range/relative motion;
- scan it;
- align a manipulator;
- grasp or dock with a simple object;
- move/release it;
- keep object state authoritative and persistent during the session.

### Slice 8 — Dedicated zero-g space test environment

Create a serious space test scene separate from the current ground sandbox.

Minimum environment:

- no flat-ground assumption;
- distant star/light source;
- asteroid or small moon reference body;
- multiple physical targets at different ranges;
- useful navigation/reference markers;
- free six-degree-of-freedom context;
- collision/contact with local bodies.

### Slice 9 — Planetary-body foundation

Introduce a spherical body model rather than treating planets as flat levels.

Foundation data/behavior:

- body center and radius;
- altitude above reference surface;
- local surface normal;
- local horizon/orientation basis;
- planetary surface collision;
- gravity field where appropriate;
- body-relative velocity;
- orbital versus surface-relative context.

Player-visible result:

- approach a spherical moon/planet from space;
- altitude and collision are meaningful;
- local “up/down” derives from the body rather than a universal world axis;
- player cannot fly through the planetary surface.

### Slice 10 — Surface / near-surface operations

Prove the same probe can operate near a physical surface:

- controlled descent/approach;
- stable hover/translation if hardware permits;
- terrain avoidance/contact;
- scan surface targets;
- manipulate a loose sample/object;
- departure back toward space.

### Slice 11 — Science as gameplay

Scanning must evolve from a countdown into increasing knowledge:

- unknown target state;
- passive observation;
- active scan;
- target classification;
- composition/material estimates;
- confidence/uncertainty;
- instrument-dependent resolution;
- repeated/longer observations improving knowledge;
- persistent discoveries;
- discoveries enabling decisions rather than merely filling a codex.

### Slice 12 — Resource/sample loop

Connect discovery to physical gain:

`identify material -> acquire sample -> store -> inspect inventory -> consume/use`

Initial scope:

- sampleable object/material;
- manipulator/tool acquisition;
- storage mass/capacity changes;
- item/material identity and provenance;
- no invisible resource teleportation.

### Slice 13 — Basic processing and fabrication

Begin Phase 4 only after object interaction and sample collection are enjoyable enough to support it.

Minimal loop:

`raw material -> process -> usable stock -> fabricate simple component/consumable`

First fabricated output should have a concrete purpose: repair, replacement, tool, sensor component, or another capability the player can immediately test.

### Slice 14 — Repair and recoverability

Use the collision/damage and resource/fabrication foundations together:

`damage -> diagnose -> acquire/process material -> fabricate/repair -> restore capability`

This is the first proof of the roadmap’s recoverability principle.

## Cross-cutting requirements

### Authoritative physics

Simulation owns mechanical truth. Unreal may supply presentation/collision queries where necessary, but gameplay-critical results must cross a defined adapter boundary and become deterministic authoritative state where practical.

### Probe-relative controls

Movement, velocity display, arms, tools, and local interaction must use understandable probe-relative frames. World-axis values may remain available for diagnostics but should not be the primary player language.

### Collision before high-speed travel

Do not increase practical travel speed or add large planetary bodies without ensuring continuous/swept collision handling is sufficient to prevent tunneling through geometry.

### Component architecture

Probe art, collision, damage, power, capability state, and manipulators should converge on identifiable components. Avoid a monolithic pawn that cannot later represent individual hardware failures, repairs, replacements, and descendants.

### Product Reality requirement

A CI-green slice is not complete if the local Unreal build still feels unchanged or the player cannot discover what was added. Every slice needs a concise local test script and visible evidence. Parallel-safe work may merge before that evidence only under the lane above; it remains explicitly Product Reality pending and cannot close the slice, phase, or release gate until the local check passes.

### Visual direction

Temporary primitives remain acceptable for isolated engineering work, but each serious embodiment test should move closer to the canonical cinematic scientific-realism target. Do not defer all visible improvement to a late “art phase.”

## Automation selection rule

When automated development is looking for the next Everward task:

1. repair any failing/open higher-priority work first;
2. keep the earliest incomplete slice as the completion/release priority; if its only blocker is a pending local Product Reality check, later work may proceed only when it qualifies for the parallel-safe lane above;
3. prefer a missing acceptance criterion in the current slice;
4. otherwise select the earliest not-complete slice in this document whose prerequisites are satisfied;
5. split it again if it cannot reasonably be implemented, reviewed, and Product-Reality-tested as one small PR;
6. do not jump to late-game systems merely because they are easier to implement in isolation.

## Definition of a solid step

A slice is a solid step when:

- behavior exists in the actual Unreal playtest, not only documentation;
- authoritative state/mechanics are covered by tests where practical;
- the player can identify the new capability without reading commit history;
- failure/rejection states are understandable;
- CI is green;
- local Product Reality evidence has been recorded;
- the next slice can build on it without rewriting the foundation.

A parallel-safe sub-slice may be merged before the local evidence is available, but it remains **implemented, Product Reality pending** and does not satisfy this definition for the full slice until the evidence is recorded.

The objective is cumulative progress: **each passed build should feel more like inhabiting and operating a real autonomous interstellar machine than the build before it.**