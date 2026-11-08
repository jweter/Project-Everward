# Everward — Claude Code and Iron God official Unreal MCP connection

**2026-11-08 · Implemented local configuration, LIVE editor/server/tool-use verification pending.**
This note is the handoff for Claude and every Iron God agent. It must **not** be interpreted as an authorization to run paid Aura Autopilot.

## Observed Azathoth configuration (checked locally, not inferred from a PR)

- Installed Unreal Engine: `C:\Program Files\Epic Games\UE_5.8\`.
- Actual working Unreal project: `C:\Users\jerem\Documents\Project-Everward\unreal\Everward.uproject`.
- Actual Claude Code **project root**: `C:\Users\jerem\Documents\Project-Everward`.
- Untracked/project-local Claude MCP config: `C:\Users\jerem\Documents\Project-Everward\.mcp.json`, URL `http://127.0.0.1:8000/mcp`, server name `unreal-mcp`.
- Project plugin declarations **locally enabled**: `ModelContextProtocol`, `EditorToolset`, `AutomationTestToolset`, `PCG`, `PCGToolset`. `AllToolsets` expressly disabled; `SemanticSearch` not enabled or configured. `Autopilot` remains enabled separately but is OWNER-ONLY.
- A backup of the original local project descriptor was saved under `C:\Users\jerem\Documents\Everward-Local-MCP\backups\`. A local, untracked setup helper is `C:\Users\jerem\Documents\Everward-Local-MCP\configure_unreal_mcp.py`.
- The .mcp.json client configuration is excluded via the **local** `.git/info/exclude` and must not be committed.
- Executed `claude mcp get unreal-mcp` from the project root: Claude Code correctly discovered this project-scoped server, reporting **Pending approval (run claude to approve)**. Therefore **client configuration exists but trust approval and live communication remain unverified**.
- At last inspection, the UE editor was **not running** and port 8000 was **not listening**. No server has been started or AI editor operation executed through the new MCP path.
- The actual laptop checkout was on `main` at `433b00eaccdb8f1aeac45cad4163dc4bd1e2ec21`, behind upstream, with existing local changes in `unreal/Config/DefaultEngine.ini`, `DefaultInput.ini`, `Everward.uproject`, and historical playtest observations. **Do not git reset/pull/checkout/merge or modify these unrelated changes for MCP setup**. Do not claim editor tests from this dirty checkout certify the newer GitHub main.

## How the connection actually works

`Claude Code in Everward project root → project-local .mcp.json → local HTTP 127.0.0.1:8000/mcp → Unreal MCP in running Unreal Editor → ToolsetRegistry → enabled Editor/AutomationTest/PCG tools.`

Official Epic UE5.8 docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor . Epic also maintains Claude Code skills at https://github.com/EpicGames/unreal-engine-skills-for-claude-code-plugin . The latter is **optional**; installation can enable a Bash SessionStart hook requiring Git Bash/WSL on Windows. Do not install or enable global Claude skills without reviewing impact on Iron God and obtaining appropriate authorization.

Unreal MCP has no API-key charges by itself, unlike optional hosted Semantic Search providers and Aura. It is **experimental, local HTTP without authentication**. Any local process with access to the loopback server may invoke available state-changing editor operations. Localhost and enabled *individual* toolsets are **not** a true read-only security boundary. Keep the listener bound only to loopback; never port-forward, expose on Tailscale/public Internet, enable unattended auto-start, or connect cloud clients directly. Do not use `--dangerously-skip-permissions`.

## Exact next steps for Claude (one controlled verification session)

1. **Review** `AGENTS.md`, `CLAUDE.md`, the current exact upstream main and open PRs, issue [#372](https://github.com/jweter/Project-Everward/issues/372), [owner-only Aura policy](OWNER_RUN_AURA_REPORT_TO_ROADMAP.md), and this document. Reconcile local dirty checkout versus current GitHub. Do not touch active worker/supervisor/leases or start Aura.
2. When the desktop owner has opened the **real** Everward project with Unreal Engine 5.8, confirm that `Unreal MCP`, `Editor Toolset`, `Automation Test Toolset`, `PCG`, `PCG Toolset` show as enabled/loaded. The editor must be restarted after the plugin enablement just made.
3. In the Unreal console, explicitly start the local development MCP server **on demand**: `ModelContextProtocol.StartServer 8000`. Keep `Auto Start Server` **off** under Editor Preferences → Model Context Protocol. The official CLI startup flag `-ModelContextProtocolStartServer` is available but **not authorized for scheduled startup**. Verify actual startup line in Output Log `LogModelContextProtocol` and loopback listener `127.0.0.1:8000`; do not claim live success until checked.
4. Launch Claude Code **from** `C:\Users\jerem\Documents\Project-Everward` (not `unreal`, Engine install folder, or an Iron God orchestration repo). Claude has already detected the project MCP config, but **is waiting for owner trust approval**. Obtain that interactive approval rather than bypassing it. Use `claude mcp get unreal-mcp` and Claude's `/mcp` to verify connection after approval.
5. Do **only non-mutating proof-of-connection calls first**: ask Claude to list registered toolsets, name the current open map, and list selected actors or inspect an existing actor; capture actual response and timestamp. With tool-search enabled, `tools/list` may show only `list_toolsets`, `describe_toolset`, `call_tool` — **this is expected**, not evidence the toolsets are missing.
6. **Never issue overlapping MCP tool calls.** Epic serializes tool invocations on the game thread; multiple agents/clients should use a single owner-controlled exclusive editor lease. The default availability of actor/material scripting does not authorize changing tracked production assets. For write/PCG generation tests, use an isolated clean feature worktree or a disposable map, with rollback, documented diffs and native UE acceptance. Do not use production game's dirty main checkout for write tests.
7. After read-only proof, verify the `AutomationTestToolset` can **discover/list** real Unreal automation tests; do not automatically run long/performance tests or inject gameplay state. Next, inspect PCG sample/reference graph rather than generating a full planet; seed each graph, preserve C++ simulation authority and test low-spec behavior.
8. When finished, stop the editor's MCP server using `ModelContextProtocol.StopServer` and verify port is no longer listening. Do not change global Claude MCP settings. If the port is occupied by another process, do not kill it: report REVIEW_REQUIRED and choose a separately approved alternate localhost port/config.

### Expected evidence report

```
repo working tree SHA + dirty status:
UE executable/build:
plugins found/enabled:
MCP server: configured / started / listener verified / stopped
Claude Code: project config found / trust approved / connected
toolsets actually listed:
selected actors or level inspection response:
automation-test discovery result:
PCG read-only inspection result:
any assets changed:
operating mode (editor read-only / isolated write test):
owner controls / memory / latency:
blockers and exact recovery step:
```

Do not claim end-to-end connection based on plugin installation, Claude saying Pending approval, a live TCP port, a synthetic test, or a PR. **Prove a real MCP tool call against the open editor.**

## Iron God integration constraints

Iron God may *later* schedule **free** native Unreal objective tests, but integration into its supervisor requires separate control/lease/idle/RAM authorization and compatibility checks. No unconditional grant to invoke an experimental editor server or to take over active owner work is established by local MCP configuration. Keep the owner `Give Me Control / Take the Wheel` gates, running worker and local memory guard intact. Do not expose the unrestricted editor MCP server directly to remote autonomous agents. A bounded, audited request broker may eventually provide approved operation classes; this document neither implements nor authorizes it.

**Aura: absolute exclusion from automated execution.** Only Jeremy can start/stop paid Aura sessions manually. Never invoke `autopilot mcp`, `autopilot connect claude`, the Aura CLI, agent service or an Aura PIE run. Read completed owner-run Aura reports offline and use issue #372 to drive narrow repairs and free native regression tests. Official Unreal MCP and Aura MCP are different services, transports and financial boundaries.

## Troubleshooting

- `claude mcp get unreal-mcp` says **Pending approval** → start `claude` in project root and explicitly approve this project MCP server. Don't bypass prompts.
- Claude says server unreachable → confirm Unreal Editor running, plugin loaded, console `ModelContextProtocol.StartServer 8000` executed, correct port/listener, local firewall and `/mcp`.
- No tools except meta-tools → normal default tool-search mode; call `list_toolsets` then `describe_toolset`.
- New toolset not visible → restart editor after enabling plugin, or run `ModelContextProtocol.RefreshTools` and reconnect client.
- Network exposure concern → `ModelContextProtocol.StopServer`; inspect loopback listener and configuration. Never enable automatic server start merely to mask a client failure.
- A Claude Code skills plugin may improve Unreal-specific tool usage, but its installation is not a prerequisite for the MCP transport itself.

**Source status: local client+plugin config verified; actual live MCP editor protocol response pending.** No Aura run launched, no paid model provider configured, no production Unreal assets edited for this integration.
