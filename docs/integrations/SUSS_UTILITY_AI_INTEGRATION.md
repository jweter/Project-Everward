# SUSS Utility AI Integration Plan

## Purpose
Evaluate `sinbad/SUSS` as a reference and potential Unreal Engine Utility AI foundation for Everward probe decision-making.

## Why it fits
Everward probes need to select among competing actions such as scan, travel, mine, repair, replicate, communicate, investigate, or flee. Utility AI is a strong fit because actions can be scored continuously against changing world state instead of being hard-coded into one rigid behavior tree.

## Core boundary
Everward owns all gameplay semantics, probe traits, save-state rules, evolution rules, child inheritance, difficulty behavior, and simulation authority. SUSS may provide an execution/scoring framework but must not define canonical game design.

## Proposed conceptual model
```text
world state
 -> candidate contexts/targets
 -> candidate actions
 -> considerations
 -> utility scores
 -> priority group / arbitration
 -> chosen action
 -> execution
 -> observation
 -> reevaluation
```

## Everward mappings
Potential considerations include:
- energy reserve;
- hull integrity;
- resource value;
- distance/time cost;
- threat level;
- curiosity/exploration drive;
- replication priority;
- mission instruction priority;
- child/parent behavior traits;
- risk tolerance;
- difficulty-mode constraints.

The Behavior Generator may influence weights, curves, priorities, or permitted strategies, but generated behavior must remain inspectable and deterministic enough to debug/replay where required.

## Phase 1: plugin/reference review
1. Review SUSS license and Unreal-version compatibility.
2. Inspect action/context/query/consideration boundaries.
3. Compare against current Everward AI architecture and Behavior Generator design.
4. Decide whether to adopt the plugin, reimplement the pattern, or use it only as reference.

## Phase 2: isolated prototype
Build a small Unreal test scene with one probe and a handful of actions:
- scan target;
- mine resource;
- repair self;
- move to safe position;
- idle/observe.

Expose utility scores and selected action in diagnostics.

## Phase 3: behavior traits
Map generated personality/behavior traits into bounded scoring modifiers. Never allow text generation itself to execute arbitrary game code.

## Phase 4: persistence/replay
Record enough decision context to explain why a probe selected an action. Where deterministic replay is required, persist random seeds and authoritative inputs.

## Acceptance criteria
- Utility decisions are inspectable in developer diagnostics.
- Behavior traits produce meaningful variation without bypassing game constraints.
- Difficulty rules remain authoritative over utility preferences.
- Save/load preserves authoritative AI state correctly.
- Plugin removal/replacement is feasible through Everward-owned interfaces.
- Performance remains within documented AI/simulation budgets.

## Non-goals
- making an LLM the frame-to-frame gameplay controller;
- allowing generated scripts to bypass safety/gameplay constraints;
- coupling core save data to plugin-specific UObject types unless explicitly approved.

## Rollback
Keep canonical probe state and behavior definitions Everward-owned. If SUSS is unsuitable, replace the scoring/execution layer without invalidating saves or design data.