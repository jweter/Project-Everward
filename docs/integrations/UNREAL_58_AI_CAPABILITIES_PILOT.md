# Everward UE 5.8: Unreal MCP + PCG + Semantic Search pilot

**Status: source-implemented gated setup; Unreal plugin enablement, MCP connection, PCG graph execution, Semantic Search provider, and on-device acceptance NOT VERIFIED.**
**Owner scope:** authorize incorporation of all three official Epic capabilities, retain Claude/Iron God review and owner-control gates, do not merge unverified work or modify other repositories.

## Why this work exists

Everward's bottleneck is not a shortage of code PRs. It is closing genuine player outcomes: the Phase 2 awakening -> scan -> tool contact -> mine -> store -> Fix_It repair -> restored capability sequence, followed by visible fabrication. An unattended editor lane can turn simulation milestones into real assets, scenes, UI feedback and repeatable Unreal evidence while avoiding another manual testing loop.

Authoritative baseline: AGENTS.md, docs/PROJECT_STATUS.md, docs/UNATTENDED_VERIFICATION.md, docs/PRODUCTION_QUALITY_GATES.md, docs/PRODUCTION_PRIORITY_BACKLOG_2026-11-05.md, docs/WORLD_BUILDING_CRAFT_PLAYBOOK.md, docs/integrations/UNREAL_AGENT_TOOLING_COMPARISON.md. The existing editor-agent comparison considered third-party alternatives; UE 5.8 now supplies an official experimental MCP implementation and should be evaluated first, **not** assumed production-safe.

## Epic sources and exact plugin names

| Capability | UE plugin declaration | Official documentation |
| --- | --- | --- |
| Unreal MCP server | ModelContextProtocol | https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor |
| MCP tool dispatch (scoped) | EditorToolset + AutomationTestToolset (ToolsetRegistry dependency automatic) | Individual toolsets are sufficient for initial editor inspection and test automation; the AllToolsets aggregator is intentionally excluded |
| Procedural content | PCG | https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-overview |
| PCG via MCP | PCGToolset | https://dev.epicgames.com/documentation/unreal-engine/working-with-pcg-and-llms-using-unreal-mcp-in-unreal-engine |
| Semantic Content Browser search | SemanticSearch | https://dev.epicgames.com/documentation/unreal-engine/semantic-search-in-unreal-enigne |

Unreal MCP is experimental, exposes a loopback HTTP server with no authentication, and dispatches editor tool calls serially on the game thread. Default editor preference Auto Start Server is false. Do not enable auto-start or expose the port to remote networks; bind loopback only. No change to local owner-control modes or autonomous task permissions is authorized by this pilot.

Semantic Search is experimental. It uses an **OpenAI-compatible vision captioning /chat/completions endpoint plus /embeddings**, is not inherently offline/free, and Epic documents that configured API keys are stored in plaintext in editor per-user settings. Saving an asset may trigger auto-reindexing. **Do not enter API keys, use a paid remote provider, index private assets, or start uncached indexing automatically.** It can stay staged but unconfigured; review provider compatibility, local-capable vision/embedding models, total RAM, and explicit owner cost approval before testing. Search is optional to completing Phase 2 gameplay.

## Verified hardware plugin inventory — Azathoth, 2026-11-08

Read-only inspection confirmed installed UE_5.8 plugin descriptors for ModelContextProtocol, ToolsetRegistry, EditorToolset, AutomationTestToolset, AllToolsets, PCGToolset, PCG, SemanticSearch, PythonScriptPlugin, EditorScriptingUtilities, GeometryScripting and FunctionalTestingEditor. The MCP, PCGToolset and SemanticSearch plugins are experimental and disabled by default; PCG is enabled by default. This verifies files **on disk only**, not that the editor has loaded or tested them.

The AllToolsets aggregator declares broad dependencies on roughly twenty-one distinct toolsets, including physics, animation, AI, Niagara, UI and other state-changing tools. The sandbox stage utility deliberately excludes AllToolsets and opts for narrower EditorToolset + AutomationTestToolset + PCGToolset. This is a reduction in exposure **not a security sandbox**; every MCP tool can still access editor state and requires an isolated worktree, agent allowlist, serialized calls and auditable action logs.

No editor launched, server started, keys configured, third-party packages installed or production assets modified during this inventory.
## Non-destructive setup contract

tools/unreal_ai_profile.py is a standard-library Python tool. No package installers, external processes or editor launch are built into it. It supports:

1. Inspect the production or disposable project descriptor without changes: python tools/unreal_ai_profile.py --mode inspect
2. After creating an **isolated linked Git worktree** on a feature branch, deliberately stage the six scoped Epic plugin declarations in that worktree only: python tools/unreal_ai_profile.py --mode stage --ack-isolated-worktree --project unreal/Everward.uproject
3. The tool refuses stage on main/master, the primary checkout, a non-linked checkout, missing/dirty uproject or missing explicit acknowledgement. This stages JSON plugin declarations only; it does not start the editor or server and does not validate that installed 5.8 plugins exist.
4. Run python -m unittest discover -s tools -p test_unreal_ai_profile.py -v, then normal repo preflight.

**Do not** run these stage actions in Iron God's active worker checkout. Use a disposable UE5.8 test branch/worktree and preserve its current supervisor, scheduled tasks, owner handoff and runtime lease behavior. The engine installation/plugin IDs must first be inspected on Azathoth. Keep Unreal MCP development-only; no shipping game server.

## Milestone 1: read-only Unreal MCP adapter

- Verify UE 5.8 on hardware contains ModelContextProtocol and AllToolsets/ToolsetRegistry; record engine build and exact Git SHA.
- With the isolated worktree editor open, leave auto-start disabled; start server on demand with ModelContextProtocol.StartServer. No external address/port forwarding.
- Generate Claude's client config only inside the disposable Unreal workspace using ModelContextProtocol.GenerateClientConfig ClaudeCode. Review generated .mcp.json for loopback-only URL and ensure it is not committed or published.
- Read only: list available toolsets, selected actors, active map, asset metadata, logs; record tool names/schema and errors with operation count/latency. Do not run overlapping MCP tool requests.
- Stop server with ModelContextProtocol.StopServer. Validate editor closes and local state is unchanged. Gate: structured transcript, no unauthorized asset diffs, no token/credentials exposed.

## Milestone 2: reversible PCG graph pilot

- Enable PCG and PCGToolset in the disposable worktree, load Epic's PCG graph generation skill if available from the installed engine; verify it actually exists instead of inventing the name.
- Inspect sample PCG graphs from Engine > Plugins > Procedural Content Framework (PCG) > SampleContent. Duplicate a working reference before creating new graphs.
- Use a fixed authoritative world seed and existing asteroid/mineral simulation facts to **project** a small bounded patch of rock/debris and a cleared mining interaction zone. PCG is scenery, not the authority for resource quantities or target contact.
- Keep point counts, mesh material count, bounds, seed and collision constraints in a small manifest. Compare same-seed transforms and object count across reruns; vary seed and verify controlled differences.
- Test extraction target remains actually reachable; capture camera/Unreal view and authoritative tool-tip-vs-surface contact. Measure generation time, draw calls, RAM/frame time on Emergency and Low. Do not author a full planetary generator before one local patch passes.

## Milestone 3: safe Semantic Search evaluation

- Verify SemanticSearch plugin loads in UE5.8 without remote provider configuration. Restrict Indexed Paths to a disposable /Game/AIResearch sample folder; disable startup/auto-save uncached indexing.
- Only after a privacy/license/cost review and a working approved local provider (or separately authorized remote cost), index a **tiny** known asset set and compare natural-language query recall against baseline folder/tag searches.
- If provider is absent, incompatible, expensive, or too heavy for the laptop, return REVIEW_REQUIRED; preserve normal Content Browser discovery. Never represent plugin enabling as successful AI indexing.
- Do not confuse indexed third-party material with permission to ship it: the existing Asset Scout license/manifest gates remain mandatory.

## First integrated value demonstration

A single selected mineral deposit in the damaged awakening map: authoritative scan -> target -> approach -> mining-arm contact -> named material -> Fix_It repair -> restored system, with a PCG-generated but deterministic visual environment, clear HUD/tool animations and unattended visual telemetry. Claude/agent uses MCP to inspect and edit **only the disposable test scene**; the existing C++ simulator controls every physical consequence. Compare duration and failure rate against the current manual/scripting workflow.

## Measurable acceptance & no-merge rules

| Gate | Proof |
| --- | --- |
| Contract | tools/test_unreal_ai_profile.py, full quality preflight, no primary-checkout writes |
| Access | plugin inventory, localhost-only MCP, read-only tool transcript, no overlapping calls |
| Reversibility | isolated uproject diff and clean rollback, no edited production assets |
| PCG | seeded graph, stable output counts/positions, valid scene contact, graphics-tier measurements |
| Search | approved/local provider or explicit REVIEW_REQUIRED; no unapproved API key or spend |
| Quality | actual Unreal build and unattended Product Reality, recorded exact head/maps/inputs/telemetry |
| Product impact | evidence of faster usable player outcome, not more PRs / screenshots / graphs alone |

Implementation states are distinct: planned -> source implemented -> Unreal verified -> product accepted. Until the laptop/Unreal checks pass, this is an **isolated feasibility pilot**. Do not merge to main or change packaged defaults.

## Claude handoff

Read this document and tools/unreal_ai_profile.py before making changes. Read AGENTS.md and current main/PR/CI/status first. Review the official Epic sources above for installation-specific differences. Reconcile plugins/dependencies against actual Engine/Plugins on Azathoth. If script assumptions are wrong, patch and add regression tests on the pilot branch; do not edit main. Report exact SHA, compiler/tests, local tool inventory, feasible read-only MCP operation, possible semantic-provider cost and RAM/VRAM risk, PCG demonstration results, and remaining Product Reality gates. Remain in the existing owner-control architecture. **No unreviewed auto-merge.**

## Rollback

Discard linked disposable worktree and branch to remove the entire experiment. No production .uproject or Unreal content files are modified by this PR until isolated staging is executed. No changes in other repositories.