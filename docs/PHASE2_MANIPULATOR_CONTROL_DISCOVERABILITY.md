# Phase 2 — Manipulator Control Discoverability

The Phase-2 manipulator mechanics already existed, but the control surface was hidden behind an undiscoverable `M` toggle. A player entering the build without reading source code or test documentation could not know the feature existed.

This pass makes the current engineering control surface self-revealing:

- the manipulator panel starts expanded on first launch;
- the panel header teaches that `M` closes/reopens it;
- the visible footer lists the complete current control set: `1/2` deploy, `3` tool, `N` arm selection, `4/5/6` joint selection, and `,` / `.` target adjustment;
- a source-contract test prevents the panel from silently becoming hidden again during Phase 2.

This is intentionally a discoverability fix, not the final mining-arm UX. The current joint-by-joint controls remain an engineering/playtest interface. Later work can replace or supplement them with a dedicated manipulator mode and end-effector/IK control without losing the requirement that controls be visible in-game.
