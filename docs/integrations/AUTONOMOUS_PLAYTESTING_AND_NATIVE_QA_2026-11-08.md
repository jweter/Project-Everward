# Everward — autonomous AI playtesting and permanent Unreal QA

**Owner direction:** build a human-out-of-the-loop game testing pipeline that joins development, authentic playable action, authoritative mechanical assertions, evidence capture, issue triage, deterministic reproduction and re-test. Preserve the Iron God supervisor, owner control gates, local-first/free builder preference and Phase-2 first playable priority. **Status 2026-11-08: evidence contract implemented on isolated branch, live interactive game agent / new native Unreal test authoring NOT yet implemented or Unreal verified.**

## Do both — sequence matters

**Lane A — objective native QA first.** Test C++ simulation with CTest (already present), run Unreal native Automation Test Framework / Functional Testing against an exact build in a disposable/approved project, collect real-state results, and optionally use Gauntlet as an orchestrator for editor/packaged sessions. Preserve the existing scheduled Windows worker's checkout safety, idle guard, build and headless startup contract. Do not replace working checks before measured equivalence.

**Lane B — interactive playtester second.** Play the real PIE game with controlled input as a novice: discover interface, target a scanned mineable body, use the selected arm and tool, obtain material and observe Fix_It. Agent can describe *where* interaction became confusing, but its natural-language opinion is advisory. Bugs must be reproduced with traces and asserted by an independent machine check before treated as fixed. Never use a hidden simulator shortcut to fake a successful interaction.

**Lane C — autonomous improvement:**

```text
authorized scope + exact main/worktree + current Product Reality
  -> single player-facing scenario and pass/fail oracle
  -> deterministic C++/native Unreal test (Agent may author tests)
  -> Unreal build + isolated PIE or packaged session
  -> optional real-time visual AI player executes gameplay controls
  -> unedited recorder events / adapter assertions / telemetry / screenshots
  -> independent evidence gate -> issue diagnosis -> bounded fix on PR
  -> exact-head preflight and repeat real playtest
  -> review + merge only when policy allows
```

## Native Epic technology available

- [Automation Test Framework](https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine): C++ functional/feature tests, Automation Driver input simulation, screenshot comparison, editor Blueprint/Python tests; register objective claims rather than only checking for crashes.
- [Functional Testing](https://dev.epicgames.com/documentation/unreal-engine/functional-testing-in-unreal-engine): actors/tests for game-level behavior and map-specific interactions.
- [Gauntlet Automation](https://dev.epicgames.com/documentation/unreal-engine/gauntlet-automation-framework-in-unreal-engine) and [Running Gauntlet Tests](https://dev.epicgames.com/documentation/unreal-engine/running-gauntlet-tests-in-unreal-engine): session and process orchestration; built-in UE.EditorAutomation / UE.TargetAutomation can invoke native engine tests. This is not automatically an AI player.
- [In-Editor Testing](https://dev.epicgames.com/documentation/unreal-engine/ineditor-testing-play-and-simulate-in-unreal-engine): true PIE input in a game session; do not confuse visual simulation with actual keyboard/controller behavior.
- [Unreal MCP](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor): agent-accessible tooling, requires isolated localhost server and correct toolset; see draft PR #367. Never assume all MCP tools safely automate PIE without empirical confirmation.

## Aura Autopilot evaluation — optional third party

Public repository: https://github.com/ramenvr/autopilot (Apache-2.0), official docs https://www.tryaura.dev/documentation/autopilot. Upstream README documents Unreal 5.4–5.8 support, an editor plugin, code-agent/MCP connectivity and game exploration/testing/verification/goal runs. **Do not install merely because the license is permissive.** It uses an agent service and decision models and documents Aura sign-in or BYOK TypeSafe API key; usage may incur service/model/subscription costs. Assess before any install: exact release hash/maintainer provenance, bundled binaries/install hooks, account and remote telemetry behavior, network boundaries, BYOK costs, secrets storage, compatibility with UE 5.8 installed from Launcher, RAM/CPU/GPU usage on Azathoth and privacy of uploaded frame/video/source data. Never enter API keys without separate authorization.

If the optional pilot cannot meet local-first/cost/privacy rules, substitute an **Everward-owned**, bounded UE functional action driver driven by fixed-step scripted player input and a separate local visual failure classifier. A cloud-powered agent should not become required to continue development.

## What was implemented in this PR

- **tools/playtest/agent_session_gate.py** — dependency-free, read-only validator for a future real Unreal session. It never opens an Unreal process, sends a model request or drives input.
- **tools/playtest/scenarios/phase2_first_repair.json** — first serious player outcome. Requires ordered actual recorder events and named **Unreal automation/functional assertions** for selected target, tip-surface contact, material gained, real energy/material spent and subsystem integrity increased.
- **tools/test_agent_session_gate.py** — offline synthetic fixtures verifying strict rejection of incomplete/mismatched/imaginary evidence. Fixture PASS is a contract test, **not a game playtest**.

Existing `APlaytestRecorderActor` emits real events to `unreal/Saved/Playtests/<session>/events.jsonl`, with `session.json`, telemetry and marker screenshots. Its currently verified Fix_It events include `fix_it_awakened`, `fix_it_runtime_bound`, `fix_it_repair_started` and `fix_it_repair_completed`. **Scan complete, mining contact, and material gain are design-required but NOT established as recorder events in current Unreal source**. Instrument them from real adapter simulation state in the next narrow slice. An agent must never append fake success events to a captured session.

### Future producer contracts (not implemented yet)

The existing recorder writes `session.json`/`events.jsonl`. A future unattended *independent runner* must write a local `run_context.json` with repository `jweter/Project-Everward`, exact 40-hex git SHA, `execution_mode=unreal_pie` or `unreal_packaged`, `/Game/...` map, deterministic integer seed, matching session ID, and scenario ID.

Native Unreal functional/automation tests must write `authoritative_assertions.json` inside the same local session (schema: object `{source_sha, scenario_id, checks:[{id, source, passed}]}`), recording each required meaningful invariant with `source=unreal_functional_test` or `unreal_automation_test`. The **independent Windows runner**, not the visual AI player, must own provenance collection and correlate exact binaries/session IDs. Ensure the assertions are bound to verifiable adapter state, not a UI label or LLM statement; do not handwrite passing assertions.

Usage after the real producers exist (illustrative only):

```powershell
python tools/playtest/agent_session_gate.py \
  --session-dir "unreal/Saved/Playtests/<session-id>" \
  --run-context "<private-worker-state>/run_context.json" \
  --scenario tools/playtest/scenarios/phase2_first_repair.json \
  --expected-sha <exact-40-hex-sha> \
  --report "<private-worker-state>/first-repair-report.json"
```

The report is sanitized and local-only unless an existing privacy-reviewed publisher projects safe fields to a GitHub issue. Status `PASS` means the **named objective event/functional-test contract** passed; it still does not certify user experience, rendered contact accuracy or artistic acceptance. `REVIEW_REQUIRED` means evidence absent/ambiguous; `ENVIRONMENT_FAILURE` means wrong checkout/commit; `FAIL` means native Unreal assertion explicitly failed. No missing evidence is promoted to PASS.

### Current outstanding dependency gates

1. Separate, clean feature worktree. Reconcile and review PR #366 (optional manipulator IK), #367 (official UE AI tooling), #368 (Epic capability audit) without importing unverified features into QA acceptance.
2. Extend `UProbeSimulationAdapter`/real Unreal gameplay event instrumentation so selection, completed scan, actual tool-tip contact and real material storage change appear as machine-readable evidence. Make the event emission read only and non-blocking.
3. Add actual Unreal engine C++ functional/automation tests that assert material and energy deltas, persisted state and repair integrity, including impossible reach and failure/retry cases. Run exact-head UBT and PIE on a disposable checkout, no manual owner step.
4. Have approved unattended worker collect matching runtime `run_context.json` plus real native assertion results. **Do not change current worker jobs, scheduler, auto-merge or owner locks until integration checks and approval confirm compatibility.**
5. First prove a deterministic scripted real-input run. Then consider bounded Aura on a separate test scene (read-only privacy review before installation) for exploration/novice-UX defects, with capped time, iteration, model cost and RAM usage.
6. Feed explicit failed checks and reproducible records into the existing Iron God issue/PR workflow. Rerun on the exact repaired commit. Compare unedited captures and frame-time on Emergency/Low; reserve the owner for subjective milestone play feel only.

## Acceptance metrics

- First-repair scenario complete in real UE5.8 PIE/packaged run with all five independent simulation assertions and evidence; target not merely a mock script. Add time/attempt counts only from measured runs.
- Same seed + commit repeat yields matching authoritative event ordering and state deltas; HUD and visibility checks captured separately.
- Negative tests demonstrate wrong SHA, missing events, stale session, fabricated/missing assertions, crash and unsupported environment cannot pass.
- Agent process memory, CPU and per-run duration measured on Azathoth; cancel safely on low-memory/owner takeover; no GPU contention with active worker.
- No periodic owner confirmation or repeated manual playtests; fail-closed output on blockers, capped retries, model spending remains zero unless separately authorized.
- Prioritized result: evidence of a better first hour of Everward—not increased PR throughput or speculative agent autonomy.

## Claude implementation brief

Read AGENTS.md, CLAUDE.md, active main/PRs, `docs/UNATTENDED_WINDOWS_PRODUCT_REALITY.md`, and this design. **Do not interpret the synthetic contract tests as actual gameplay PASS.** Run existing CI first; promote one verifiable micro-slice at a time. Extend recorder events and native Unreal tests, then connect through dedicated existing worker safely. Evaluate Aura only after the privacy/license/cost review. Never create provider accounts, install opaque packages, alter scheduler/owner controls, or merge code without applicable verification and standing authorization.

## Rollback

The current PR adds a standalone Python validator, synthetic tests, and one scenario manifest. It changes neither production game code nor the active automation worker, and is removed by reverting those files.