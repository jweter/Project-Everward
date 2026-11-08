# Everward: Epic Unreal Engine 5.8 capability audit (first broad pass)

**Review date:** 2026-11-08. **Status:** researched and prioritized; no engine plugins installed or verified merely by this document.

## Decision

Do not install every useful-looking plugin. Use Epic's own shipped tools to reduce the time between a **player-visible acceptance outcome** and independently verifiable Unreal evidence. The first milestone is still Phase 2's damaged-awakening -> scan -> targeted mining contact -> named materials -> Fix_It repair -> restored capability. Everything here must serve that player loop or remain a staged future candidate.

Repositories reviewed at audit start: current main 5fe75561432b31d25b6f4a25b683b14a7367f13d, docs/PROJECT_STATUS.md, AGENTS.md, docs/PRODUCTION_PRIORITY_BACKLOG_2026-11-05.md, docs/WORLD_BUILDING_CRAFT_PLAYBOOK.md, docs/integrations/UNREAL_AGENT_TOOLING_COMPARISON.md, docs/PROCEDURAL_MECHANISM_STUDIO.md and docs/UNATTENDED_VERIFICATION.md. Some status prose contains historical SHAs; the actual branch head/CI/issue inventory outrank stale prose.

## Tier P0 — highest impact on producing verified playability now

| Epic technology | Existing Everward work | Gap to close | Smallest validated experiment |
| --- | --- | --- | --- |
| **Automation Test Framework**, Functional Testing, Automation Driver, screenshot comparison | C++ headless tests, existing unattended Windows build and headless smoke | The game still needs complete unattended interaction tests and visual evidence | Record one real scenario: scan -> tool contact -> mine -> inventory -> Fix_It; assert state in Unreal and capture actual HUD screenshots with commit and seed |
| **Gauntlet** session runner | Batch scripts + unattended Windows worker | More structured packaged-play smoke, process lifecycle and logs, beyond merely opening the editor | Isolated packaged smoke test with existing scripts as baseline; compare setup and debugging complexity, do not rewrite already reliable tests |
| **Unreal Insights / Trace**, Stat Commands, Console Variables Editor | LowSpec/Emergency presets, RAM/process survival tests | Frame-time, CPU/GPU bottlenecks, asset costs and input hitches not consistently measured on target laptop | Capture a reproducible 30-second awakening/mining sequence for Low/Emergency and log p50/p95 frame time, working set and stutters |
| **Official Unreal MCP / Toolset Registry** | docs/integrations/UNREAL_AGENT_TOOLING_COMPARISON.md discussed third-party editors | Safely automate editor inspection and creation rather than only text code | Read-only localhost pilot in isolated worktree; PR #367 stages a guarded feasibility harness, not production activation |

Sources: https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/gauntlet-automation-framework-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/testing-and-optimizing-your-content ; https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor

## Tier P1 — high leverage for immediate visual and interaction quality

| Epic technology | Existing Everward work | Gap to close | First test |
| --- | --- | --- | --- |
| **Modeling Mode and Geometry Scripting** (Beta) | Procedural Mechanism Studio, GLB exports and Prime A reference assets | Making and refining usable geometry, pivots, collision and materials inside Unreal | In disposable scene, reconstruct ONE articulated arm module or docking part; test proportions, pivots, collision and change/reimport determinism against source C++ oracle |
| **PCG Framework + PCGToolset** | World-Building Craft Playbook already specifies seeded, geological placement and hero composition | Repeatable authored environment details that agents can build inside UE | Seeded 20-100 object debris/mineral patch with collision-free extraction zone, benchmark draw calls and layout repeatability, retain simulation-owned material values |
| **Sky Atmosphere + sky materials; Volumetric Cloud** | Everward atmospheric-space visuals and newly discussed Instant Skies inspiration | Believable planetary vistas without purchasing a Unity-only asset | One airless-moon baseline using sky material; one physically justified atmosphere preset only on appropriate body, capture fixed cameras at Low/Emergency; avoid costly clouds for vacuum |
| **Niagara** | Continuous Visual Quality requires realistic physical effects | Contact-driven drilling dust/particles, sparse debris and thruster effects, not generic looping particle decoration | Emit a capped contact-triggered mining effect strictly from authoritative events, prove no VFX on false reach/no contact |
| **Common UI + Enhanced Input** | Manipulator HUD and controls discoverability, plans for remapping | Contextual on-screen actions, input routing, gamepad/mobile readiness without competing controls | A single target/arm interaction panel that displays real current bindings and safe manual override |

Sources: https://dev.epicgames.com/documentation/unreal-engine/modeling-mode-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/introduction-to-geometry-scripting-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview ; https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/creating-visual-effects-in-niagara-for-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/common-ui-plugin-for-advanced-user-interfaces-in-unreal-engine

## Tier P2 — useful when the dependent milestone arrives

| Epic technology | Roadmap use | Constraint / non-displacement |
| --- | --- | --- |
| **MetaSounds and Audio Insights** | Sensorium and machine-driven mining/repair feedback; audio engine already has an Everward design plan | Event-driven perception in airless space, no decorative soundtrack or independent audio game truth; measure CPU and accessibility |
| **StateTree** | Task-focused drones, worker behavior, repair action presentation | Keep authoritative task logic and physics in the engine-independent simulation; inspect if tree is useful as a presentation/orchestration projection |
| **MassEntity / MassGameplay** | Dense *nearby* fleet representation as player creates many drones | Do not migrate canonical identity/resource/lineage simulation into Mass just for speed; benchmark only after real fleets exist |
| **World Partition + HLOD, PCG hierarchical generation** | Moon surfaces and industrial complexes at larger scales | Not a solution to interstellar coordinate truth; avoid in Phase-2 single-room starter level unless measured benefit |
| **Semantic Search** (Experimental) | Finding imported mesh/material/texture assets in large library | Requires OpenAI-compatible captioning + embeddings provider; API keys stored plaintext; provider calls can cost money. Stage unconfigured and do not index automatically |
| **Movie Render Queue / Movie Render Graph** | High-quality references, visual change reviews and game trailers | Offline footage is never proof of real-time game performance or a completed playable milestone |

Sources: https://dev.epicgames.com/documentation/unreal-engine/metasounds-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/overview-of-state-tree-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/overview-of-mass-entity-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/world-partition---hierarchical-level-of-detail-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/semantic-search-in-unreal-enigne ; https://dev.epicgames.com/documentation/unreal-engine/movie-render-pipeline-in-unreal-engine

## Tier P3 — presently deprioritized or conditional

- **Gameplay Ability System**: powerful for abilities/effects, but a large conceptual replacement of deterministic mining/repair commands would increase dual-state complexity. Evaluate only if a specific unavoidable problem emerges.
- **Learning Agents, large AI/ML NPC stacks**: current intelligent drone goals can be rule-driven; importing ML behavior now risks nondeterminism and CPU/memory cost. Keep as later research.
- **Epic marketplace/Fab plugins and third-party procedural assets**: licensing, provenance, dependency and offline QA remain under Asset Scout. Never install a free asset merely because the store says free.
- **Full cinematic render stack**: useful for promotional material once an integrated playable scene exists, not a substitute for improving the game.

Source: https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-system-for-unreal-engine

## How to measure whether an Epic feature accelerates milestones

Before adopting a candidate, record:

1. **Exact user-facing outcome** (e.g., 'the player sees the correct tool physically reach the deposit and obtains measured material').
2. **Existing baseline**: steps/toolchains required, repeated manual intervention, number of failed runs, CPU/GPU/RAM and required input.
3. **Smallest repeatable test** in a disposable worktree and map, with fixed seed/asset provenance, exact source hash and low-spec preset.
4. **Output artifact**: .uasset/Blueprint/PCG graph or command/script + screenshots, replay, logs, numeric telemetry and a visible pass/fail oracle.
5. **Dependencies**: license, cost, installed UE plugin, editor threading, permissions and nondeterminism.
6. **Promotion gate**: repeat success with zero owner-driven routine tests, no simulation source-of-truth violations, no gameplay regression, and real improvement versus baseline.

Suggested scorecard columns: feature, player-outcome dependency, existing coverage, source doc, risk/cost, independent test, estimated integration work, measured saved work, winner/reject/defer, and review date. Do not fabricate saved hours or benchmark wins.

## Proposed sequence after current PR review

1. **Finish the actual broken/untested opening gameplay loop** and capture accepted automated Unreal evidence (higher priority than any tool adoption).
2. Use official **Automation Test Framework + Unreal Insights** to turn that integrated loop into a measured, repeatable gated test suite. Compare with the existing Windows worker before replacing it.
3. In parallel, review draft PR #367's guarded official Unreal MCP / PCG / Semantic Search integration. Claude must reconcile exact installed UE5.8 plugin IDs, safety, the sandbox worktree and no-spend rule. No premature live-server activation.
4. If editor read-only pilot passes, produce **one PCG mineable environment patch** with deterministic simulation-owned contact and a side-by-side gameplay capture, then assess actual speed improvements.
5. Test **Geometry Scripting / Modeling Mode** on the canonical Prime A arm and equipment components; scale up only if asset quality and round-trip pivots improve.
6. Add contact-driven **Niagara / MetaSounds** and contextual UI once mechanics produce truthful events; defer Mass/StateTree/World Partition until their underlying gameplay systems exist.

## Follow-on: real AI game playtesting and durable native QA

**2026-11-08 source verification:** The public [Aura Autopilot](https://github.com/ramenvr/autopilot) project is Apache-2.0 and documents Unreal 5.4–5.8 compatibility with editor PIE control and optional Claude/Codex integration. Its onboarding starts a model/agent service and may require an Aura account or a TypeSafe BYOK key; do not conflate open-source software with zero-cost or private operation. The vendor's reported benchmark is not Everward-specific.

**Decision:** build test-owned truth first, then visual play agent. PR [#369](https://github.com/jweter/Project-Everward/pull/369) adds a fail-closed recorded-session evidence gate and a staged Unreal native-testing plan without installing Autopilot or modifying the existing Windows worker. The test fixture is synthetic and its pass is not evidence of a playable game. It is intentionally independent of the experimental official editor MCP pilot in [#367](https://github.com/jweter/Project-Everward/pull/367).

Read-only inspection of Azathoth's installed UE_5.8 plugin descriptors confirms official ModelContextProtocol, ToolsetRegistry, AllToolsets, EditorToolset, AutomationTestToolset, PCGToolset, PCG, SemanticSearch, GeometryScripting, PythonScriptPlugin and FunctionalTestingEditor on disk. Presence is not proof of editor activation. Avoid exposing the broad AllToolsets aggregator initially.

Use this dependency order: exact-head C++ and native Unreal Functional/Automation test -> independent adapter assertions and recorder event stream -> bounded real-input PIE session -> optional isolated Aura exploratory run -> issue/fix/CI/Unreal re-test -> gameplay Product Reality. Only the independent functional assertions can certify mechanical outcomes; subjective feel remains a milestone review.

Official Epic references: [Automation Tests](https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine), [Gauntlet](https://dev.epicgames.com/documentation/unreal-engine/running-gauntlet-tests-in-unreal-engine), [PIE](https://dev.epicgames.com/documentation/unreal-engine/ineditor-testing-play-and-simulate-in-unreal-engine).
## Scope of this audit

This is a broad **targeted, first-pass audit** of the highest-value documented UE5.8 subsystems for Everward, not a page-by-page crawl of Epic's entire documentation and not proof any subsystem was installed or tested. Continue with deeper subsystem-specific audits only when the roadmap reaches their dependency gates. Every follow-on should link to an actual roadmap item and measured outcome.

**Claude/Iron God review gate:** read AGENTS.md, inspect current main/open PRs and Product Reality before acting, preserve all owner-control gates. Do not automatically merge this research into a claim that Unreal capabilities were operational.