# Everward Playtesting

Everward treats laptop playtests as a **continuous validation lane**, not a development phase gate.

Development continues along the roadmap while human playtests periodically verify the actual Unreal build on real hardware. A playtest result may change priorities, but ordinary feature failures do not automatically stop unrelated development.

## Result classes

- **PASS** — feature behaves as intended.
- **PARTIAL** — feature is usable but has defects or UX problems; log and prioritize appropriately.
- **FAIL** — feature is broken; record evidence and fix according to severity while unrelated development continues.
- **BLOCKER** — continuing dependent work would build on invalid state (for example: project will not launch, save corruption, foundational input failure, or simulation-state corruption).

Only BLOCKER findings stop dependent development.

## Unreal recorder

`APlaytestRecorderActor` is spawned automatically by the production `AEverwardGameMode` in `unreal/Everward.uproject`.

The recorder is deliberately non-blocking. If it cannot start, Unreal logs a warning and gameplay continues.

Each run writes raw evidence beneath:

```text
unreal/Saved/Playtests/<UTC-session-id>/
├── session.json
├── events.jsonl
├── telemetry.csv
└── screenshots/
```

`session.json` records the session ID, engine/build metadata, start/end state, and issue-marker count.

`events.jsonl` is append-only structured event evidence. The recorder itself emits lifecycle and issue-marker events. Gameplay systems should gradually add feature-specific events by calling `RecordPlaytestEvent`, for example:

```text
scanner_target_selected
scan_started
scan_completed
mining_arm_command
mining_contact
material_collected
fix_it_repair_started
fix_it_repair_completed
```

The recorder is evidence only. It must never determine simulation truth or gameplay outcomes.

`telemetry.csv` currently samples basic frame and player-pawn motion evidence at 2 Hz. Additional probe state should be added incrementally as systems become real (energy, temperature, storage, scanner state, mining state, arm state, etc.).

## F12 issue marker

During a playtest, press **F12** when something looks or feels wrong.

Do not use F9 for playtest markers. In Unreal Editor PIE, F9 conflicts with the editor's Wireframe view-mode shortcut; the 2026-08-30 laptop playtest demonstrated this by switching the viewport into wireframe during evidence capture.

The recorder will:

1. append an `issue_marker` event,
2. request a screenshot into the current playtest session,
3. increment the marker count in `session.json`, and
4. show a short on-screen confirmation.

This lets the tester continue playing instead of stopping to reconstruct the issue immediately.

## Mining control validation

Current Phase-2 mining controls intentionally preserve player choice while adding optional navigation assistance:

```text
N     cycle selected manipulator arm
7     attach/detach mining tool on the selected arm
H     cycle available mining targets
P     engage/cancel auto-approach to the selected surveyed mining target
G     attempt extraction
SPACE stop propulsion and cancel auto-approach
```

Auto-approach only navigates to an arm-side staging position. It does not articulate the manipulator or extract material. The player retains control of the arm and mining action.

Phase 2 currently exposes one mineable bootstrap target. Target cycling is already modeled as a list-based control so additional scanned resource bodies can join the same workflow without changing the input contract.

## José Take the Wheel validation

**José Take the Wheel** is the general-purpose smart autopilot. It is separate from mining auto-approach: José gets the probe from point A to a player-selected destination, while mining auto-approach handles specialized arm-side work positioning.

```text
T       cycle/select physical destination
Y       engage/cancel José Take the Wheel
SPACE   emergency stop / manual takeover
WASDQE  manual translation / immediate takeover
```

For the current first playable slice, verify:

1. select a physical target with `T`;
2. press `Y` and confirm the actual probe begins moving toward it;
3. confirm the persistent HUD line shows José as engaged and names the destination;
4. confirm commanded speed reduces as surface range approaches the configured stand-off;
5. confirm José stops rather than colliding with the selected body;
6. during transit, press `SPACE` and confirm manual control returns immediately;
7. re-engage, then press a manual translation key and confirm manual control returns immediately;
8. change the selected target during transit and confirm José stops instead of continuing toward a stale destination;
9. verify a rejected propulsion command produces a readable autopilot-stop message rather than silent failure.

The canonical design and future navigation progression are defined in [`JOSE_TAKE_THE_WHEEL.md`](JOSE_TAKE_THE_WHEEL.md).

## Raw evidence versus repository artifacts

Do **not** commit complete `Saved/Playtests/` sessions to Git. Raw screenshots, logs, crashes, and telemetry can become large quickly.

The desktop playtest launcher may collect those files into the user's external playtest archive.

When a compact report is useful, run:

```powershell
python tools/playtest/summarize_session.py "unreal/Saved/Playtests/<session-id>"
```

That creates `playtest_report.md`, which is small enough to attach to an issue or preserve with a targeted observation when appropriate.

## Development priority handling

Playtest evidence feeds normal work selection roughly as follows:

```text
critical regression / BLOCKER
        ↓
high-value player-facing defect
        ↓
current roadmap feature
        ↓
minor UX defect
        ↓
polish
```

This is guidance, not an excuse to stop roadmap progress for every observed defect.

## Instrumentation rule

New player-facing systems should expose useful structured playtest events when practical, but instrumentation must remain loosely coupled. A recorder outage must not change gameplay behavior.

The long-term objective is to make every laptop play session reproducible engineering evidence while preserving continuous development velocity.
