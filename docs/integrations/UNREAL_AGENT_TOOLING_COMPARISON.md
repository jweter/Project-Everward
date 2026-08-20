# Unreal Agent Tooling Comparison: UE-MCP vs Monolith vs Hayba

## Purpose
Evaluate AI-agent control surfaces for Unreal Engine development so coding agents can inspect, modify, test, and diagnose the Everward project with less manual editor work while preserving repository safety and review gates.

Primary candidates:
- `db-lyon/ue-mcp`
- `tumourlove/monolith`
- `zajalist/hayba`

## Scope
This is **development tooling**, not gameplay/runtime AI.

Potential capabilities to evaluate:
- inspect project/assets/Blueprints;
- create or modify approved assets;
- query reflection/API metadata;
- inspect navigation, behavior trees, EQS, perception, GAS, UI, materials, Niagara, audio, Sequencer, and PCG where supported;
- launch PIE or automated tests;
- read build/editor logs;
- produce structured diagnostics for agent workflows.

## Safety boundary
Agent tooling must never receive unrestricted authority over `main` or release artifacts.

Required operating model:
```text
agent request
 -> capability allowlist
 -> isolated feature branch/worktree
 -> Unreal/editor operation
 -> diff/asset-change inspection
 -> automated tests
 -> human/CI review
 -> merge only when green
```

## Evaluation matrix
Score each candidate on:
- Unreal version support;
- setup complexity;
- breadth of exposed actions;
- read-only inspection quality;
- write granularity;
- Blueprint support;
- C++/reflection support;
- test/PIE/log control;
- structured error reporting;
- permission/capability controls;
- auditability;
- rollback/recovery;
- maintenance activity;
- license/commercial compatibility;
- risk of destructive editor operations.

## Default principle
Prefer the **smallest capability surface that solves the workflow**. A tool exposing 1,000+ actions is not inherently better if it creates a larger failure surface.

## Phase 1: read-only trial
Start with read-only tasks:
- query project settings;
- inspect assets/components;
- inspect Blueprint structure;
- read logs;
- inspect current AI/navigation configuration.

No writes until read behavior is predictable.

## Phase 2: disposable write sandbox
On a dedicated branch/test map only:
- create a trivial Blueprint/component;
- modify a non-authoritative test asset;
- run validation;
- revert/recreate from source control.

Measure whether changes are understandable in Git/source-control review.

## Phase 3: constrained development tasks
Allow narrow actions such as:
- adding a test component;
- wiring a known Blueprint node sequence;
- updating a test data asset;
- executing PIE and returning logs.

Every mutation must be attributable to the agent/tool action.

## Phase 4: choose one primary tool
Do not integrate all three. Select one primary Unreal-agent interface based on measured reliability and maintainability; retain the others as reference candidates.

## Acceptance criteria
- Read operations are reliable and structured.
- Writes occur only on isolated branches/test assets until explicitly promoted.
- Changes can be reviewed and reverted through normal source control.
- Tool failure cannot silently corrupt authoritative maps/assets.
- CI/testing remains the merge authority.
- No credentials or private data are exposed through the tool surface.

## Rollback
Disable/remove the agent tooling without affecting the packaged game. All authoritative game content remains normal Unreal/project content under source control.