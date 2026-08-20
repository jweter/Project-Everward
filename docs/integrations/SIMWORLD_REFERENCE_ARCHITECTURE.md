# SimWorld Reference Architecture

## Purpose
Study `SimWorld-AI/SimWorld` as a reference architecture for separating Unreal world simulation, agent observations, planning, and actions in Everward.

## Posture
Reference-only by default. Do not make SimWorld a runtime dependency unless a later prototype proves a compelling reusable component with acceptable license and maintenance characteristics.

## Reusable architectural idea
```text
Unreal world state
 -> observation/perception layer
 -> normalized agent state
 -> planner/decision layer
 -> action request
 -> game-authoritative validation
 -> Unreal action/execution
 -> new observation
```

This separation is valuable for long-lived autonomous probes because gameplay state should remain authoritative even if the decision implementation evolves.

## Everward-owned contracts
Define stable interfaces for:
- observations available to a probe;
- knowledge versus hidden world state;
- action requests;
- action validation;
- execution outcomes;
- errors/failures;
- persistence/replay metadata.

The agent must never receive omniscient world information unless a game mechanic explicitly grants it.

## Planning boundary
Possible future planners may include:
- deterministic rules;
- Utility AI;
- behavior trees/state machines;
- hierarchical planning;
- bounded generative planning for high-level goals.

All planners should emit requests through the same game-owned action boundary.

## Prototype
Create a minimal test harness where a probe can:
1. observe a limited local system state;
2. choose among scan, move, wait, and investigate;
3. submit an action request;
4. have Unreal validate cost/range/permissions;
5. execute or reject it;
6. persist the resulting decision trace.

## Acceptance criteria
- Agent observation never bypasses fog-of-war/scan/gameplay rules.
- Planner implementation can be replaced without rewriting core world simulation.
- Invalid actions fail explicitly.
- Decision/action traces are inspectable for debugging.
- Save/load preserves authoritative world state independent of planner internals.

## Non-goals
- adopting an AI research simulator wholesale;
- making an external model authoritative over Unreal state;
- exposing every game subsystem directly to an LLM.

## Rollback
Because the architecture is interface-based, any experimental planner or agent layer can be disabled while the underlying Unreal simulation remains intact.