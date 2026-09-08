# Fix_It Unreal implementation notes

`Fix_It` is a core game/lore system and must remain simulation-authoritative.

The current Phase-2 bridge intentionally keeps the engine-independent `FixItPlanner`, `FixItRepairExecutor`, and `FixItReplacementExecutor` as the source of truth. Unreal orchestration may expose state, schedule fixed-step advancement, seed a Product Reality awakening configuration, and record events, but it must not duplicate repair economics or priority rules.

Fresh Phase-2 sessions use the damaged-awakening integrity profile only as the current playable opening fixture. Save/load remains authoritative after the session has begun; the runtime actor detects a replaced simulation runtime and rebinds without repeatedly reseeding damage.

The current on-screen Fix_It line is temporary presentation. The active Phase-2 playability/HUD pass should eventually replace it with a first-class readable Fix_It panel while retaining the same underlying simulation state and lifecycle events.
