# Aura Autopilot integration — Everward UE5.8 (2026-11-08)

**Status: plugin downloaded and staged ONLY in an isolated local test clone. No login, agent-service installation, live Unreal editor test, session or gameplay acceptance yet.** See [PR #369](https://github.com/jweter/Project-Everward/pull/369) for independent fail-closed gameplay evidence; [PR #370](https://github.com/jweter/Project-Everward/pull/370) for authoritative scan/material events. Do not collapse those into a claim that an AI player actually tested the game.

## 2026-11-08 operational update — OWNER-ONLY paid runs

**Current verified state:** Aura v1.0.0 was installed in the **real** Azathoth Everward checkout at `%USERPROFILE%\Documents\Project-Everward\unreal\Plugins\Autopilot\`; its editor modules loaded in UE5.8.1. Jeremy completed account setup and manually initiated the first genuine exploration on 2026-11-08. Run `20261108-084846` executed 3 episodes/666 decisions, reported approximately $4.45 estimated model usage, and found multiple first-playable blockers. See [#372](https://github.com/jweter/Project-Everward/issues/372) and the comment on [PR #371](https://github.com/jweter/Project-Everward/pull/371).

**New binding owner policy supersedes outdated feasibility steps below:** **Jeremy alone initiates every Aura session; Claude, ChatGPT, Iron God and scheduled/unattended workers must never start or schedule paid Aura testing.** Do not run `autopilot connect claude` for autonomous access or create any service-based test triggering. AI development agents may read completed reports (with owner-authorized access), convert them into measurable roadmap slices, fix game code, and use existing unpaid native/Unreal regression tests. Lack of a new Aura run never blocks independent development. Full operating procedure: `docs/integrations/OWNER_RUN_AURA_REPORT_TO_ROADMAP.md`.

**The remainder of this file records the original installation experiment and earlier proposals, not authorization to automate Aura.** All older clauses suggesting Claude-submitted Aura scenarios, an Iron God Aura worker, a ChatGPT-driven local Aura runner or ongoing API/model spending are **withdrawn** by this update.

## Why this rather than more isolated feature PRs

Reduce manual testing while improving the actual Phase 2 first-playable loop. Autopilot should **play the real game** and identify camera, accessibility, controls, mining reach, input discoverability and Fix_It problems. The independent Unreal simulation + native functional assertions must prove material/energy/sensor/integrity state, regardless of anything an LLM reports. Successful PR CI, an Autopilot report, and genuine player-visible acceptance are separate gates.

## Pinned official release and observed local facts

- Upstream: https://github.com/ramenvr/autopilot (Apache-2.0; Unreal plugin marked Beta; Win64 UE 5.8).
- Exact release: v1.0.0-9e0fbb0, published 2026-11-06. The pinned 50,625,623-byte official ZIP and SHA256SUMS.txt were downloaded to Azathoth; local SHA-256 matched `1e5954087c5d70b03bb25a5d22b92420fe87966131b928fa07402db0002c8e8e`.
- Unpacked ZIP contains 155 entries (about 238.6 MB expanded), three Unreal modules (AutopilotPlaytest, AutopilotPlaytestEditor, AutopilotCodex), bundled binaries, and agent-service installers. Files passed path-traversal and symlink checks.
- Isolated installation root (ONLY): `%USERPROFILE%\Documents\Everward-Aura-Pilot\Project-Everward\unreal\Plugins\Autopilot\`; an independent checkout with branch `pilot/aura-autopilot-ue58-sandbox-20261108`. The local `Everward.uproject` plugin list was changed in this clone only. The plugin folder is Git-ignored. Primary `Documents\Project-Everward` checkout and the sentinel-protected unattended worker remain untouched.
- `python -m unittest discover -s tools -p test_aura_autopilot_pilot.py -v`: **7/7 passed** locally before staged installation. These are script contract tests, not Unreal runtime tests.
- No Unreal Editor/agent service has started and no account/API key or payment has been provided.

## Why the first playtest is not automatically free or private

The upstream docs say an **Aura account** or **BYOK TypeSafe API key** is needed for actual decision-model playtesting. Aura advertises a free two-week trial, $10 premium credits, and no card required (verify live billing details before enabling ongoing use). Its plugin installs a separate agent service when onboarding begins. On Windows, its sandbox is a *child process*, not a Linux-style network/filesystem isolation boundary. Autopilot may process project source, frame images, reports and token usage; choose no sharing while evaluating. Its `autopilot connect claude` changes the **user's** Claude Code MCP settings, skills and agents; it is not a project-only change. Do not run this globally in Iron God's service account without review.

Public sources:
- https://www.tryaura.dev/documentation/autopilot
- https://www.tryaura.dev/autopilot
- https://github.com/ramenvr/autopilot/blob/main/plugin/unreal/Autopilot/README.md
- https://github.com/ramenvr/autopilot/blob/main/autopilot/connect.py
- https://github.com/ramenvr/autopilot/blob/main/autopilot/sandbox.py

## Owner account setup (human-only, one time)

1. On Azathoth, **close active Unreal/UBT sessions** first; do not interfere with the existing scheduled worker. Open only `Documents\Everward-Aura-Pilot\Project-Everward\unreal\Everward.uproject` in Unreal Engine 5.8. This project is intentionally disposable and independent from the actual active build checkout.
2. Confirm the **Autopilot** orange-controller icon appears, or open **Window > Tools > Autopilot**. If it does not load or triggers a DLL failure, do not modify the main project; preserve a redacted editor log for follow-up.
3. Click **Create Account**, or open https://www.tryaura.dev/ and create/sign into the Aura account on the official domain. Do not paste account passwords or tokens into ChatGPT, PRs, issues, or Git. Complete any email verification yourself.
4. Resume plugin onboarding and let it set up the agent service only once you accept the local installation and account terms. Prefer Aura's initial trial instead of BYOK paid API keys. Choose **do not share runs/training data** for the private development pilot. Check trial balances and usage caps before further runs.
5. When **Signed in** / **Agent service ready** appears, click Continue/Finish. **Skip** the automatic ten-minute exploration initially: Azathoth has ~16 GB RAM, and interactive Unreal + extra model services may cause memory pressure.
6. Run ONE bounded verification prompt in the disposable project: "In the Everward Phase-2 starter map, identify available scanner/mining controls, attempt to scan the mineable bootstrap deposit, and report whether the probe can physically get its selected mining arm/tool within valid contact distance. Do not modify source or project assets. Capture real gameplay frames, action log, and mark any failure INCONCLUSIVE if the world or input is not accessible. Stop after 3 minutes." Turn sharing off.
7. Keep the resulting local report, episode recording, editor log, test commit, engine version, and memory data separate from the main Git tree. **Do not** mark the milestone accepted based on the AI narrative alone; reconcile with `APlaytestRecorderActor` and native assertions.

If the trial is depleted, agent service fails, or resource usage is too high, disable Autopilot in the disposable project and retain the permanent native QA lane without manual re-testing.

## Claude Code connection

Official supported command **after completing onboarding**: `autopilot connect claude`. This writes a user-scoped MCP entry and Claude skills/agents. **Prefer a project-scoped configuration first**: in the disposable project only, an ignored `.mcp.json` may name `autopilot mcp` as a local stdio server. Claude must be launched *from that isolated project* and approve/trust the MCP entry. Do not automatically modify global `~/.claude` or the Iron God development supervisor.

Use Claude in two roles: (a) author native Unreal tests/fixes on independent reviewed branches, (b) submit one bounded Autopilot scenario and independently compare its report to simulation-driven events. No simultaneous Unreal MCP and Autopilot editor operations. Do not let the AI tester make gameplay true by changing cheats, save files or recorder traces.

## Iron God / development-engine integration

Integrate as a separate, **owner-approved QA worker/lane** once a single manual-onboarding pilot passes. The orchestrator receives a limited request (`commit_sha, scenario_id, seed, max_minutes, local_artifact_dir, allow_network_auth, mode`). It must check owner Take-the-Wheel authority, idle/memory thresholds, active editor/worker locks, model-budget approval, exact clean disposable checkout, and serialize exclusive Unreal access.

Run the real-play/autopilot phase with a fixed scenario, collect local private raw evidence, invoke `tools/playtest/agent_session_gate.py` against independent Unreal native assertions, and publish only sanitized result status / reproducible failure references to existing issues. Avoid changes to orchestrator privileges, scheduling, merge rules, local secrets and other repositories in this PR. A production QA-lane adapter remains follow-on implementation, not claimed complete here.

## ChatGPT access

ChatGPT **cannot directly attach to Aura's laptop-local stdio MCP process** as a normal ChatGPT MCP app. ChatGPT custom MCP apps generally require a remote MCP endpoint and have plan/permission restrictions; do not expose Autopilot's localhost server to the public internet. In this conversation, the connected **Remote Desktop Commander** app already permits **authorized inspection and commands on Azathoth**, and GitHub provides PRs and sanitized evidence; these form the ChatGPT bridge for coordinating and interpreting Aura results. ChatGPT should invoke the local worker only after account/service onboarding and any necessary tool permissions, not by claiming a direct native Aura connection.

A longer-term adapter could expose a **small, authenticated read-only report/queue endpoint** via approved secure tunnel if the account/ChatGPT plan supports that. Never forward Aura credentials, raw private captures or arbitrary local code execution through a public MCP server.

## Verification / rollback

A working pilot must demonstrate: UE5.8 plugin loads in isolated project; account service actually connected; actual PIE session controlled by Aura; raw episode actions, screenshots, native recorder evidence and selected target state; error classification and safe process cleanup; memory/time/cost measurements; no changes to primary project or scheduled worker; no global Claude configuration edits unless separately approved.

Rollback: close Unreal, remove `unreal\Plugins\Autopilot` from the **isolated** checkout, restore only the isolated `Everward.uproject` from Git, and optionally remove that clone and the separate release cache. If the agent service was installed by onboarding, use Autopilot Settings > Uninstall first (and review whether memories should be preserved). Do not delete the original active development repo or private worker.

**Implementation state:** downloaded + checksum verified + staged isolated plugin; **UE/runtime account/Claude/Iron God/ChatGPT live playtest integrations still pending**.
