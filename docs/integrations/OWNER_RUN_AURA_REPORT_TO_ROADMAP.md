# Owner-run Aura, report-driven Everward development

**Owner decision, 2026-11-08:** Aura Autopilot is an **owner-operated, paid exploratory playtesting tool**. Jeremy alone initiates, configures, and ends Aura sessions inside Unreal. Claude, ChatGPT, Iron God, scheduled jobs, CI, and unattended workers **must not start, schedule, continue, retry, or issue model/tool requests to Aura**, including via its CLI, MCP server, agent service, editor UI, or scripts. The mere presence of Aura credentials, an installed plugin, a connected MCP server, or an earlier owner authorization to use the plugin is **not authorization to spend credits**. Do not add Aura invocation to DoWork, unattended workers, auto-refill, preflight, GitHub Actions, or any other recurring job. A new explicit owner decision is required to change this boundary.

**The development engine remains autonomous without Aura:** after Jeremy finishes a session and makes the report available, Claude/ChatGPT/Iron God may *read* the existing local report and related existing Unreal playtest records, analyze source, prioritize work, author tests, implement repairs, perform normal free/local C++ and UE objective tests, open issues/PRs, and verify results. Never let lack of a new Aura session block ordinary development.

## Immediate override: complete all first-run defects before any new Aura run

The owner has explicitly required **complete correction and independent Unreal verification of the entire 2026-11-08 first-playtest finding list before another paid Aura run**. The detailed authoritative release gate is [#378](https://github.com/jweter/Project-Everward/issues/378), items G01–G13. This supersedes any earlier suggestion to re-run after just the mining geometry fix, a single passing PR, or first Unreal MCP connectivity. Continue free/local deterministic engineering and native Unreal tests; **do not propose or trigger paid Aura re-testing while any G-item remains open**. Even after closure, Jeremy alone decides whether to conduct an exploratory run.

## Before a manually initiated Aura run: Unreal MCP evidence preflight

The owner has now connected to **official Unreal MCP from a local terminal**. For future paid exploratory runs, first verify the *current* editor connection with an actual read-only MCP result, record exact build/map/dirty-tree and reachable starting-state evidence, and ensure the Everward recorder is ready. During the owner-run session, do **not** issue concurrent Unreal MCP operations that could disrupt Aura's control of PIE. After completion, reconcile existing Aura report **plus** authoritative Unreal recorder/events and optional MCP postflight. Details: `UNREAL_MCP_AURA_PLAYTEST_EVIDENCE_WORKFLOW_2026-11-08.md`.

An MCP connection itself does not supply a continuous gameplay trace or justify a factual acceptance claim; missing MCP capability is recorded as missing evidence. MCP preflight and report consumption are **not authorization for any agent to invoke the paid Aura service**. Jeremy may choose to test without a completed MCP preflight; such a run must be labelled accordingly rather than hidden or automatically repeated.

## Evidence chain

1. **Jeremy runs Aura manually, on his schedule.** If he wants help forming a bounded prompt, produce a draft for him to run; never submit it to Aura. He owns any account/credit-cost decision, optional video/data-sharing choice, and stopping the session.
2. **Report becomes an engineering input, not an automated command.** When notified of a completed run, read the saved `report.json`, `findings.md`, and necessary traces from local authorized sources, plus corresponding `unreal/Saved/Playtests/` session records. Keep raw captures, passwords, logs containing tokens, and account identifiers on the laptop. Do not publish full transcripts or videos to public GitHub; post a sanitized finding summary and local evidence pointers only. If reports are incomplete, mark that explicitly.
3. **Compare against exact source and historical defects.** Record run ID, date, engine/build, tested Git SHA **and dirty-worktree status**, input scenario/seed, episode counts, agent claims, actual runtime measurements, reported token usage/cost *as an estimate*, and known related GitHub issues. Do not call a dirty/out-of-date worktree the current-main Product Reality. Distinguish independently observed failures, agent-reported suspicions, code-only hypotheses, and expected behavior/test-harness errors. Explicitly cross-check whether the issue was already fixed on newer main.
4. **Convert observations into small roadmap slices.** For every meaningful finding record: player-visible symptom, why it blocks a player goal, priority (P0 broken first-playable loop; P1 correctness/UX; P2 polish or tooling), reproducer, likely owning subsystem, verified evidence and uncertainty, dependency, *single measurable outcome*, and exact independent regression oracle.
5. **Keep one issue as the run's index** (first example [#372](https://github.com/jweter/Project-Everward/issues/372)) and create focused linked implementation PRs for cohesive fixes; deduplicate against the current backlog. State `REPRODUCED`, `SOURCE-HYPOTHESIS`, `EXPECTED/TEST-ERROR`, `IMPLEMENTED`, `CI_VERIFIED`, `UNREAL_VERIFIED`, or `PRODUCT_ACCEPTED` as applicable. An issue checkbox never implies completion without a cited exact commit and evidence.
6. **Fix and test without Aura.** Prioritize the earliest failing real player outcome; implement small changes with C++ unit/contract tests, Unreal native Functional/Automation tests, and the existing unattended low-spec Windows worker. Record actual simulation/tool-tip/inventory/repair effects rather than relying on model commentary, a button press, or a synthetic event.
7. **Close with evidence, not new paid exploration.** Update linked PR and issue with exact merged SHA, CI/unattended Unreal result, regression evidence, remaining Product Reality uncertainty and any manual visual/novice-play acceptance pending. Mark `AURA_RETEST_PENDING_OWNER` only when a subjective exploratory re-run is useful; it is *not* a blocker on otherwise independent work and does not authorize any agent to trigger Aura. Jeremy chooses whether and when a future paid run is worth it.

## First report: already indexed

- Owner-initiated **Aura run `20261108-084846`**, 3 episodes, 666 decisions across 27 controls; reported model usage **about $4.45** (estimated; actual account charges require checking provider dashboard). Unreal first-playable loop not certified; mining failed on reach/approach/joint-control preconditions and no named material was mined.
- Engineering tracker: [issue #372 — reproduction evidence, causes, classifications and repair/re-test checklist](https://github.com/jweter/Project-Everward/issues/372).
- Aura installation/adoption PR: [#371](https://github.com/jweter/Project-Everward/pull/371) (merged). Native event evidence: [#370](https://github.com/jweter/Project-Everward/pull/370) (merged). Evidence validation contract: [#369](https://github.com/jweter/Project-Everward/pull/369) (merged).
- The following are **separate, tractable work packets**, not one speculative "fix mining" ticket:

| Packet | Player outcome | Required objective proof | Status on 2026-11-08 |
| --- | --- | --- | --- |
| P0: mine after scan and approach | The deployed, tool-equipped selected arm can reach a surveyed deposit and extraction increases named inventory/decreases deposit | real simulator tool-tip/reach plus successful `G` with positive material delta; Unreal automated input and feedback | BLOCKED / investigate reach & controls; agent did not articulate joints |
| P1: scanned discovery persists | A successful scan remains known after a subsequent scan is cancelled | scan completed, then new scan cancelled, `bSurveyed` stays true | REPRODUCED |
| P1: arm toggle reverses on both transitions | Player can interrupt either deploy or stow to reverse the arm | input during stowing results in deploying; tests both directions | REPRODUCED |
| P1: collision fidelity | The probe hull and bodies obey the authoritative compound collision/contact solver | exact compound hull tests incl. moving body; distinguish false positive 8m sphere heuristic | INVESTIGATE |
| P1: save/reload material consistency | Save restores named material, deposit, survey, tractor/targets consistently | before/after state round-trip with actual mining success | SOURCE-HYPOTHESIS; no successful mining was observed |
| P2: bounded HUD index | Hidden/open systems panels never allow invalid capability selection | spam `]` with panel closed and verify bounded selected index | REPRODUCED |
| QA: reliable local regression | Fixed-input, bounded test attempts scan/reach/mine/repair before owner spends on exploration again | normal native Unreal and C++ tests, fails closed on missing events | PARTIAL / not complete |

## Template for future owner-provided findings

```markdown
### [P0|P1|P2] Short player-facing symptom
- Aura run ID / observed episode & timestamp:
- Tested commit & local modifications (and why still relevant to current main):
- Reproduction (inputs, seed, map, preconditions):
- Objective observation vs agent explanation vs unverified hypothesis:
- Impact on first-playable dependency:
- Candidate owning files/system:
- Exact acceptance oracle (positive + negative, no fabricated events):
- Implementation PR / tests / Unreal evidence:
- State: REPRODUCED | SOURCE-HYPOTHESIS | EXPECTED/TEST-ERROR | IMPLEMENTED | CI_VERIFIED | UNREAL_VERIFIED | PRODUCT_ACCEPTED
- Cost of owner Aura run (estimated vs dashboard-confirmed):
- Aura re-run: NOT REQUIRED | OWNER-MAY-CHOOSE (never scheduled)
```

## Explicit non-goals

No unattended Aura launcher, MCP invocation from an agent, account/credential integration, cost-trigger automation, Aura polling daemon, global Claude Aura connector, or paid playtesting runs. Building a *report parser* for existing saved report files is permitted if it is **offline, read-only, does not start a service, and never publishes raw secrets**. This policy overrides earlier suggestions to have Iron God run Autopilot as a QA lane; those proposals are superseded, not a standing permission.

**Ownership split:** Jeremy = paid exploratory testing. AI development agents = free/local verification, engineering work, documentation, prioritized follow-up and evidence review. 
