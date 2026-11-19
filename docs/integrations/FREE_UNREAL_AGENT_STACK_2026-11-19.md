# Everward — free UE 5.8 agent stack

Status: source implemented and unit-tested in a feature PR. No claim that
Unreal MCP is connected right now or native Unreal gameplay acceptance passed.

## Pinned skill references

The full 31-skill Unreal Engine 5.8 collection from
https://github.com/quodsoler/unreal-engine-skills is included at
.agents/skills/ue-*/ with all 90 Markdown files and its MIT license.
Pinned upstream commit: f3742d7b688690810df369802b90430324e380b9.
The local UPSTREAM_LOCK.json records exact SHA-256 hashes of the source
Markdown and license. The free runner verifies the installed lock.
No third-party executable scripts are installed or run.
These reference skills are not execution authority: AGENTS.md, project
truth, the owner gate, CI, reviews and separate simulation truth win.

## Official UE 5.8 MCP

The previously deployed, experimental editor configuration already
selects ModelContextProtocol + EditorToolset + AutomationTestToolset
+ PCGToolset. It excludes AllToolsets intentionally.

Use: python tools/unreal_free_agent_stack.py --mode mcp-read

This contacts ONLY localhost 127.0.0.1:8000/mcp to initialize and list
tools. It never calls the arbitrary tool dispatcher, writes an asset,
starts the editor/server, or enables a permanent listener. The owner
must authorize the separate live session. A successful tools list does
not itself establish editor mutation safety or gameplay correctness.

Epic's optional Claude Code skill plugin and other third-party UE MCP
servers are not activated: the native Epic implementation is primary,
and our free local route must not consume Claude/Aura tokens.

## Native test, build, packaging and Gauntlet profiles

This runner supports only Epic's installed Unreal tools:
- ubt: EverwardEditor Win64 Development through Build.bat
- automation: UnrealEditor-Cmd, NullRHI, native Automation RunTests prefix
- gauntlet: RunUAT RunUnreal using an existing named Gauntlet test
- package: RunUAT BuildCookRun Win64 Development output

Plan without executing:
  python tools/unreal_free_agent_stack.py --mode plan --kind ubt
  python tools/unreal_free_agent_stack.py --mode plan --kind automation --test Everward
  python tools/unreal_free_agent_stack.py --mode plan --kind gauntlet --test Everward.Smoke
  python tools/unreal_free_agent_stack.py --mode plan --kind package

Execute ONLY in an explicitly authorized, clean, **linked isolated Git
worktree**, never the owner's primary or dirty checkout:
  python tools/unreal_free_agent_stack.py --mode run-native --kind automation --test Everward --ack-isolated-worktree --evidence-dir C:\EverwardAgentEvidence\run-001

It requires actual UE binaries, free disk >= 10 GiB and physical RAM
available >= 7 GiB, a single-instance process lock, explicit acknowledgement
and a separate evidence directory. It records exact SHA, process result,
test report presence, timestamp and bounded log. Standalone execution
is a developer opt-in tool; autonomous invocation must still use Iron
God's owner-grant, exclusive worker lease, admission, supervised runtime
and release gates.

Process return code zero is NOT a verified functional-test verdict.
Native results are REVIEW_REQUIRED pending assertion/report inspection,
and they cannot merge or assert Product Reality.
Screenshot comparisons require actual screenshot tests/baselines;
Gauntlet requires a real project-specific registered test.

## Permanent exclusions and constraints

No paid Aura; no automatic paid Claude; no hosted Semantic Search,
API key submission or provider indexing; no broad AllToolsets;
no free-form Python editor actions; no automatic permanent MCP startup;
no unsafe resource admission; no uncontrolled game-state mutations.
Keep simulation authority in the headless deterministic C++ core.
Use low-spec Product Reality and idle windows on Azathoth.

Reference existing policies:
- docs/integrations/CLAUDE_UNREAL_MCP_CONNECTION_HANDOFF_2026-11-08.md
- docs/integrations/UNREAL_MCP_AURA_PLAYTEST_EVIDENCE_WORKFLOW_2026-11-08.md
- docs/UNATTENDED_VERIFICATION.md
- docs/TESTING_STRATEGY.md

## Unit checks and next native gate

  python -m unittest discover -s tools -p test_unreal_free_agent_stack.py -v
  python tools/unreal_free_agent_stack.py --mode inspect
  python tools/check_foundation.py

The subsequent admitted local exact-head gate must confirm real Unreal
build/test availability, report-level assertions, scan -> target -> mining
tool contact -> material accounting -> Fix_It repairs, telemetry,
low-spec memory and visual evidence. Missing results remain pending.
