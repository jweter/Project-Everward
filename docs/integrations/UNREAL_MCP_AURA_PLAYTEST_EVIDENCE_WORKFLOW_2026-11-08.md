# Everward: Unreal MCP preflight → gameplay test → evidence-driven repair

**Owner direction, 2026-11-08:** Before an owner-chosen paid Aura exploration session, establish a *verified* **official Unreal MCP** connection so the development system captures reproducible facts from the actual Unreal build, not only the playtester narrative. After the session, analyze BOTH Unreal-owned evidence and the completed Aura report to create causal diagnoses, small roadmap slices, regression tests and verified repairs.

**Owner-only payment boundary remains absolute.** Jeremy alone starts, operates and ends Aura. No Claude, ChatGPT, Iron God, scheduled job, CI, MCP adapter or autonomous worker can start, control, retry, schedule, or incur fees for Aura. Unreal MCP is an independent local editor bridge, **not** Aura MCP. Offline analysis of a completed owner run, and *non-Aura* deterministic tests, remain authorized. See `OWNER_RUN_AURA_REPORT_TO_ROADMAP.md`.

## HARD GATE before any *future* paid Aura playtest (owner direction 2026-11-08)

**As of this instruction, NO additional paid Aura playtest is in scope.** The owner requires **EVERY item** from the first run fixed or conclusively ruled out with independently measured evidence **before** another paid exploration can even be proposed. Full authoritative checklist: [Issue #378 — Aura Retest Release Gate](https://github.com/jweter/Project-Everward/issues/378), items **G01 through G13**.

The MCP preflight/postflight procedure below is a *future capability*, **not** a reason to run Aura now. During the current engineering cycle, implement and execute the same MCP/recorder/Unreal tests using **free local native tests only**. Close all G-items with targeted regression and actual in-editor evidence at the accepted exact candidate build; merge and verify fixes with no unresolved critical behavior. If evidence disproves a suspected bug, document why, including the correct hull/test methodology. Issue #372's prior closure and merged PR #375 do not satisfy unverified G-items.

**Only after all G01–G13 pass** may an optional new owner-run exploratory session be discussed. Even when issue #378 is closed, Jeremy personally controls timing, approval and all paid Aura execution; no agent ever runs Aura.

## Why use both sources?

- **Unreal MCP (editor source of truth):** Discover the opened project, map and available read-only inspection tools, capture the target build/level/config and preconditions; when supported, inspect actor positions and game/simulation state. After the run, inspect the same build and available level state. MCP's evidence is a snapshot, not automatically continuous gameplay telemetry.
- **Everward-owned recorder / runtime telemetry (authoritative chronology):** Saved playtest session metadata, recorder `events.jsonl`, telemetry, crash/command logs, state transitions, and native Unreal or simulation assertions. This is the primary evidence for *what the game actually did* while the test ran.
- **Aura (exploratory behavior and observations):** Owner-run input/decision trace, failed tasks, confusion, bug hypotheses and report. Aura's narrative is not authoritative simulation truth, and it does not make an acceptance test pass by itself.
- **Git / CI / engine tests (reproducibility):** Exact commit and dirty-tree/asset fingerprints, source provenance, C++ unit tests and native Unreal Functional/Automation assertions. These validate *why* it happened and whether a fix holds.

**Important:** The toolsets exposed by Epic's Unreal MCP may include mutating operations. "Read-only" below is an *allowlisted task/permission rule*, not a guaranteed technical read-only server mode. Never invoke a write-class tool on the real dirty development checkout.

## Stage A — PREPARE (before any owner Aura run)

1. **Open the actual Everward project** in UE5.8, then manually/owner-approved start `ModelContextProtocol.StartServer 8000` if not running. Claude Code launches from `C:\Users\jerem\Documents\Project-Everward` with the project-local `.mcp.json` targeting `http://127.0.0.1:8000/mcp`. The owner has reported a successful local terminal connection; the first setup document's older "pending approval" status is a historical baseline, **not proof of current disconnection**. For each new testing session, obtain an actual real editor tool response before marking `MCP_CONNECTED`. Do not silently bypass Claude project trust.
2. **Read-only MCP preflight:** confirm this is the intended project, engine version, open level/map, available toolsets and relevant actors/components. Prefer `list_toolsets`, `describe_toolset`, then only known read-only `call_tool` operations. Do not enable `AllToolsets`, cloud Semantic Search or unrestricted remote access. Stop and mark `MCP_PRECHECK_FAILED` if a live tool cannot return the expected facts. Ordinary free development continues even if it fails; an Aura session is always Jeremy's optional choice.
3. **Capture exact environment:** `git rev-parse HEAD`, branch, `git status --porcelain`, tracked/untracked build-affecting changes, clean vs dirty test artifact, engine/editor version, plugin versions, map and target body IDs, scenario/seed, expected gameplay objective and bounded owner-defined duration. The local main checkout has previously been stale and dirty; **never reset/pull/rebase it automatically** to make a run "clean".
4. **Record starting gameplay state when the available tools allow:** probe pose and attitude, current health/integrity, system power and energy, sensor/target scan state, target positions and radii, selected arm/tool, joint deployment and reach, tractor state, inventory/deposit amounts, and Fix_It objective/repair prerequisites. If MCP cannot inspect PIE or authoritative state, record `MCP_STATE_UNAVAILABLE`; use the Everward recorder and native source-of-truth tests instead. Do not manufacture readings.
5. **Confirm evidence capture:** check the Everward recorder is available and will create a new session under `unreal/Saved/Playtests/`; preserve timestamps and run/session ID. Capture source-file or event-contract version. Do not enable private video/data sharing without owner's choice.
6. **Release exclusive editor control for the paid run:** stop automated MCP **calls** and any other editor-controlling client before Aura begins. It is acceptable to leave the local MCP server reachable while the editor stays open, but do not issue concurrent tool commands; never have Claude/MCP and Aura manipulate the same PIE session. Prefer one explicit "editor owner" lease where infrastructure supports it. Preserve Iron God owner-control gates, memory floors and existing worker activity.

**A preflight is a useful information-gathering step, not permission for an agent to launch Aura.** If the owner wants to test without a live MCP snapshot, label evidence limitations honestly rather than blocking their choice.

## Stage B — OBSERVE (owner manually runs Aura; or separate native unattended tests)

- Jeremy elects and runs any **paid Aura** session himself, with its own time/cost limit. Never automatically start/retry an Aura process, use `autopilot mcp`, or connect the paid agent to Claude.
- Independently collect **continuous game truth** via the game's already implemented recorder, appropriate runtime telemetry/command logs and true native test assertions. Do not confuse an Aura screenshot, button press, acceptance of a command, or approximate probe envelope with proof of success.
- Correlate event clock and run identifiers. Track scanning complete/cancel, selected target, approach/staging, chosen arm, deployed/stowing fraction, joint pose and real tool-tip/surface gap, contact events (compound hull vs bounding sphere), mining accepted/rejected, material/deposit and storage deltas, save/load, and Fix_It repair cost, integrity and restored capability.
- If an input never fired, a required precondition was missing, a sensor lost power, an arm failed to articulate, or tool reach remained invalid, report that precisely instead of attributing failure to the wrong subsystem.
- Resource guard: do not start additional large LLM/editor workers simultaneously on Azathoth's 16GB RAM. No automatic paid retries on timeout.

## Stage C — RECONCILE (after owner ends paid session)

1. Confirm Aura is no longer driving the editor. Where permitted, reconnect to the official Unreal MCP **for read-only postflight** on the same project, query current map/target/actor state and verify that recorder sessions/logs exist. A fresh editor/restarted PIE world can reset runtime state; distinguish an actual post-test state from a newly initialized scene. Never treat an editor snapshot as historical trace of an earlier PIE session.
2. Read completed local `%LOCALAPPDATA%\Autopilot\memory\Everward\runs\<run-id>\report.json`, `findings.md`, necessary bounded trace excerpts, and `unreal/Saved/Playtests/<session-id>/` evidence. Sanitize before GitHub; **do not publish raw source dumps, private captures, tokens or account metadata**.
3. Map Aura timeline to recorder/simulation timeline and exact build. Produce a linked **evidence table** distinguishing:
   - `REPRODUCED` — actual gameplay/state deltas + relevant logs and reliable oracle;
   - `SOURCE_HYPOTHESIS` — plausible code path, not yet runtime-proven;
   - `HARNESS_LIMITATION` — Aura automation or test-precondition problem;
   - `EXPECTED` — documented intended behavior;
   - `ENVIRONMENT_FAILURE` — tooling/connection/RAM/build failure;
   - `INCONCLUSIVE` — missing, noncomparable or stale-build evidence.
4. Reconcile current GitHub main and existing issues/PRs **before** new work. An observed older dirty checkout failing does not prove the current source has the same bug. Historical first Aura evidence is linked in [#372](https://github.com/jweter/Project-Everward/issues/372), but its present status may be closed; check its resolution before reopening or duplicating.
5. Make **small player-outcome roadmap slices**, ordered P0 blocked first playable → P1 game correctness/reliability → P2 UX/polish/QA. Each slice records symptom, source build, reproducer, why it affects the player, hypothesized cause, one correction, expected objective outcome, exact regression oracle, owner and dependency, associated PR/issue and evidence status.
6. **Fix without Aura:** use C++ simulation/unit tests, Unreal native Automation/Functional tests, already approved unattended Windows worker and precise error reporting. Re-test the exact candidate commit/asset state. Status is `IMPLEMENTED` or `CI_VERIFIED` until actual target-build Unreal behavior is verified. Keep `AURA_RETEST_OPTIONAL_OWNER` for worthwhile future exploratory confirmation; do not schedule or make it a per-PR gate.
7. Write a **sanitized durable development note** to the appropriate issue/PR, with file/commit references and concise pre/post facts. Publish only after source comparison and regression facts are verified. The goal is a higher-quality playable experience, not simply more tests or PRs.

## Minimal per-session record

```text
record_version: 1
aura_owner_run_id: <run-id or NOT_APPLICABLE>
unreal_recorder_session_id: <id or MISSING>
date_utc: <date>
project: Everward
git_sha: <exact tested local sha>
git_branch_dirty_status: <branch, dirty, sanitized diff fingerprint>
unreal_version_map: <editor version, map/level>
unreal_mcp_preflight: CONNECTED | FAILED | NOT_RUN
unreal_mcp_evidence: <toolset/tool name + actual response identifiers, or UNAVAILABLE>
initial_state: <measured values, or NOT_ACCESSIBLE>
test_objective: <player outcome>
run_clock: <start/end times, timeline alignment>
events_and_telemetry: <local artifact references, observed actual deltas>
aura_report: <local report refs, hypotheses, reported estimated cost>
postflight_state: <measured/reinitialized/not accessible>
evidence_classification: <REPRODUCED/etc>
small_roadmap_slices: <issue IDs, verification oracles>
next_retest: FREE_NATIVE | OWNER_OPTIONAL_AURA | NONE
```

## Access policy and operational boundaries

- **Real project**: `C:\Users\jerem\Documents\Project-Everward\unreal\Everward.uproject`, **MCP project root** `C:\Users\jerem\Documents\Project-Everward`.
- Official MCP at `http://127.0.0.1:8000/mcp`, on demand; keep localhost-only and avoid overlap with third-party local tools or Iron God workers. Never port-forward it.
- Prefer the existing five narrow plugins (`ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset`, `PCG`, `PCGToolset`); `AllToolsets` disabled. Never enable external semantic/cloud model providers by assumption.
- **No changes to owner-control flags, global Claude MCP settings, service credentials, scheduled tasks or automated Aura execution.**
- A failed/MISSING MCP preflight means **MCP-enhanced evidence is unavailable**, not automatically "gameplay failed" or "user must test again."
- Preserve standalone authoritative test evidence and human subjective judgment as separate layers. Read-only editor observations support but never replace simulation invariants, exact-build native assertions and playability evaluation.

**Completion criterion for this protocol:** owner can initiate any optional Aura run *after a successful real MCP read-only preflight*, and later Claude/Iron God can trace each important reported defect back to actual local event/state evidence, current source and an independently verified issue/repair without starting another Aura session. Document itself is an operating contract, not proof that this pipeline has been executed end to end.
