# Everward — Contextual Guidance and Action Discovery

**Status:** Proposed canonical design; awaiting PR review and merge  
**Priority:** P0, first-playable experience  
**Parent:** #384 — Full in-game HUD and interaction redesign  
**Owner direction:** The player must always understand what is possible, what has purpose, what to try next, and how to perform the action without memorizing a scattered command list.

## 1. Product promise

**The game gives direction; the player chooses the destination.**

Everward is an open-ended game in which the player *is the probe*. A complex simulation is a feature, but obscuring affordances is not. The interface should answer six questions at any moment:

1. Where am I, and what condition am I in?
2. What object or system am I currently interacting with?
3. What actions are possible **right now**?
4. What meaningful objective could I pursue?
5. Which concrete next action advances that objective, and why?
6. Did the attempted action work? If not, what is preventing it?

The guidance system is **not** a mandatory scripted quest chain, not an omniscient narrator, and not an autonomous pilot. It must preserve choice, discovery, and manual mastery.

## 2. Design principles

- **World first.** Keep the default view spare. Guidance lives in a compact, dismissible objective cue and a contextual action tray; full details are requested, not permanently displayed.
- **Simulation determines truth.** Suggestions, ranges, required materials, disabled reasons, and rewards derive from authoritative state, never invented UI text.
- **Purpose → next step → action → feedback.** Surface a useful goal, the nearest actionable dependency, its input, and its observed outcome.
- **Recommend, don't command.** The player can ignore, dismiss, pin, change or disable guidance. Do not claim there is a single correct path.
- **Progressive disclosure.** Show a small set of relevant immediate actions, with a way to see all controls, advanced system telemetry and possible alternatives.
- **Explain blockers.** Differentiate unavailable, out of range, equipment missing, power insufficient, not scanned, damaged/offline, busy, undiscovered and genuinely locked.
- **Embodiment.** Guidance is framed as the probe's operational advisory capability, eventually incorporating Fix_It. It should be credible for the sensor and computation hardware currently available.
- **Accessible and rebindable.** Plain-language action names, symbols plus text, scalable UI, keyboard/mouse focus, and extensible controller navigation.
- **Respect uncertainty.** If a target composition is unknown, say so. A damaged sensor can report uncertainty without falsifying an objective.

## 3. The three-layer interface

### A. Always-on minimal HUD

Only essential health, energy, active mode/tool, selected target (if any), navigation status, critical alerts, and one optional **Next Action** cue. No default wall of debug strings or full-height subsystem panels.

Suggested compact card:

> **OBJECTIVE — Restore mobility**  
> Fix_It estimates propulsion repair requires material not yet in storage.  
> **NEXT:** Scan a nearby resource body. **[Scan key]**  
> **Why?** Identify whether accessible material can support repair.  
> **Change goal · Hide hint · Details**

Descriptions and key names are illustrative, **not** hard-coded gameplay claims.

### B. Contextual action tray

On target or mode change, show relevant supported actions, their actual live input bindings, availability, requirements, and response to activation.

Example: **Unsurveyed body**

| Action | Condition | Explanation |
|---|---|---|
| Scan | Available if working sensor and valid target | Identify mineable material |
| José: approach | Available if navigation and propulsion permit | Move to safe stand-off |
| Mine | Blocked until target is surveyed and arm/tool/range permit | Show actual unmet requirement |
| Tractor | Depends on installed tool, effective range, and physics | Show real state |

After a successful scan, display the revealed information and updated options; do not pretend the scan always finds iron.

### C. Optional in-depth panels

Dedicated Science, Navigation, Manipulator, Systems, Storage/Industry and Fix_It views retain detailed diagnostics and logs. A single **Actions / Controls** entry point shows **all current actions**, categorized and searchable in future, including current bindings and why each action is unavailable. Do not require a separate manual.

## 4. Objective and recommendation model

Define a gameplay-agnostic objective representation:

- `objective_id`, category, origin (`player`, `fix_it`, `system`), optional target entity ID
- objective description and player-facing reason
- evaluated prerequisites / dependencies and authoritative evidence
- current step, candidate actions, alternatives, and next-step priority
- confidence / unknowns, status (`available`, `blocked`, `in_progress`, `complete`, `invalidated`)
- optional pinned/dismissed state in player preferences
- observable completion condition and timestamp or state revision

A guidance evaluator reads authoritative state through read-only queries. Its recommendation output is a **presentation proposal**, not an instruction with permission to change the simulation. Command execution remains in existing gameplay command handlers. Existing José autopilot and Fix_It systems continue to own their respective actions and safety gates.

Suggested selection policy:

1. Respect the player's pinned objective, if valid.
2. Otherwise prioritize survival-critical needs and immediate hazards, with transparent explanation.
3. Suggest progress toward recently chosen player targets or active tasks.
4. Offer relevant opportunities uncovered by scans or interaction.
5. If nothing urgent or meaningful is known, say **Explore / Choose a goal** rather than creating busywork.

Show **one primary recommendation** and at most a small number of alternatives. If no valid next action exists, show the exact blocker, fallback actions, or an explanation that more information is required.

**Re-evaluate** after meaningful state changes: selection, sensor result, movement/range transition, arm/tool change, resource acquisition, subsystem change, action success/failure, hazard event, goal change. Avoid expensive per-frame recomputation and prompt flicker; debounce appropriate transient updates.

## 5. First playable walkthrough — damaged awakening

This is an example of intended *conditional* guidance, not a promise every generated scenario has iron or an identical repair bill.

1. **Awaken:** Present survival/energy condition and nearest feasible recovery objective. Explain which systems are damaged; don't obscure the world.
2. **Observe/scan:** Guide the player to select a visible resource body and activate a functional sensor. Clarify target selection controls.
3. **Interpret result:** Report verified composition and uncertainty. If unsuitable, recommend a different surveyed/unsurveyed body.
4. **Approach:** Present José as an *option* alongside manual flight. Explain whether propulsion can act and what approach stand-off means.
5. **Prepare manipulator:** Show which arm is deployed, selected joint, attached tool, target reach vs actual tool-tip-to-surface distance, and how to change the selection.
6. **Mine:** Enable extraction only when surveyed-target, tool, range, storage and other real prerequisites pass; otherwise explain precisely why.
7. **Collect:** Confirm change in stored inventory, with actual quantities and capacities.
8. **Repair:** Fix_It recommends the most valuable feasible restoration consistent with its survival/restore-before-polish doctrine. Indicate required material, energy, time and progress; permit detailed inspection.
9. **Regain capability:** Update available actions, acknowledge observable system improvement, suggest a meaningful new opportunity.
10. **Continue:** Offer alternate paths such as further survey, storage, manufacturing, experimentation, or user-defined goals.

At no point should the player need to hunt through a large hotkey dump to find the next relevant command.

## 6. Action discovery behavior

Every registered action should have stable ID, localized label, icon, category, description, current binding, applicability predicate, disabled reason, optional associated objective and invocation target. The same action registry must drive prompts, control reference and input help to prevent contradictions.

When an action fails, provide a concise, actionable explanation such as:

- `Mining arm is out of reach: tool tip 15.52 m from selected surface; maximum reach X m.`
- `Target must be surveyed before extraction.`
- `José unavailable: propulsion offline.`
- `Storage full: X / Y kg.`

Here `X` and `Y` are authoritative values obtained at runtime. No silent failures. A generic error may include an expandable technical diagnostic, but should never replace player-facing guidance.

## 7. Future scaling

The same model supports player-directed objectives (e.g. “fabricate one scanner drone”), industrial bottleneck recommendations, logistics, construction, descendants and doctrine-level management. Fix_It evolves from a self-repair adviser to an industrial adviser, but cannot fabricate goals or exert control without player authorization and existing gameplay rules.

As the probe evolves, additional interface capabilities can become *available* according to real hardware and knowledge. Basic action discovery, reliable input help and clear error reporting must **never** be artificially broken as a progression gimmick.

## 8. Visual and interaction design

- Restrained industrial/scientific type and iconography; strong contrast, readable at normal 1080p viewing distance.
- Target markers sized by importance, not giant world-space headlines.
- Noncritical hints time out or collapse; pinned objective remains accessible.
- The same input must not both close an overlay and trigger an unintended gameplay command.
- No pointer-only interaction; all primary actions work via keyboard and can be mapped.
- HUD hide/photo mode suppresses noncritical prompts without losing game state.
- Support 1920×1080 and 1280×720 layouts, UI scale settings, and low-spec performance.
- No daily-user-facing debug diagnostics unless diagnostic mode is explicitly activated.

## 9. Implementation slices and dependencies

**Slice 1 — Audit and contract (P0):** Inventory existing Unreal UI, keybindings, gameplay commands and source-of-truth state. Produce action/availability matrix and reference layouts. Determine whether commands need adapters; avoid rewriting simulation.

**Slice 2 — Action registry and controls discoverability (P0):** Establish one source for action name/binding/applicability/blocker; create the Controls / Actions screen and responsive context tray. Include regression tests for binding changes and disabled reasons.

**Slice 3 — Objective evaluator and Next Action cue (P0):** Implement compact objective presentation, prerequisite graph/dependency stepping, goal pin/change/dismiss, and state-change recalculation.

**Slice 4 — First-loop integration (P0):** Verify live scan, José, manipulator, mining, inventory and Fix_It behavior as the game implements them. No UI-only simulated successes. Capture evidence and correct confusion points.

**Slice 5 — Production UX hardening:** Accessibility, controller preparation, automated screenshots and interaction regression, user-testing protocol, and instrumented first-session failure/recovery traces.

Keep each change isolated, CI-checked, and in scope for Everward's current phase and Iron God owner-control boundaries.

## 10. Acceptance gates

- A first-time player can identify, without external instructions, **what is selected**, **what is available**, **what is recommended**, **how to activate it**, and **why a desired action is blocked**.
- A real first-playable run supports discovery through scan → optional approach → tool setup → extraction → inventory → Fix_It repair whenever underlying mechanics make that path possible.
- The UI never shows an action as available if authoritative state would reject it for a known prerequisite.
- Input bindings displayed in context and in the controls view match the actual active input mapping; conflicts are tested.
- All significant actions have observable success, progress, or actionable failure states.
- Objective hints change when state changes and never remain stale after completion or invalidation.
- Dismiss, pin, change objective, HUD-hide, UI scaling and disable-guidance options work and persist appropriately.
- 1080p and 720p automated screenshots show no clipping, overlap or large default diagnostic panels.
- Regression evidence protects existing manual controls, José, tractor beam, mining and Fix_It behavior.
- Usability sessions measure task completion, wrong actions, time to discover controls and whether players understand the *purpose* of each step. Code coverage alone does not satisfy UX acceptance.

## 11. Explicit exclusions

This milestone does **not** implement an LLM narrator, autonomous gameplay decision-maker, new physics, new repair economics, all future civilization management UI, or a mandatory quest campaign. It establishes the truthful interaction and guidance foundation those systems can later reuse.

## 12. Project interfaces

- Parent implementation issue: #384.
- Align with the existing **You Are the Probe** pillar, **continuous visual quality** standards, José navigation controls, Fix_It recovery doctrine, and future unbounded machine control.
- Update the canonical roadmap and Phase 2 execution plan **through reviewed PRs** to identify this guidance/UI work as a first-playable gate; do not wait until late-game UX polishing.
