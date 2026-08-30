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

`APlaytestRecorderActor` is spawned automatically by `ABenchmarkGameMode`.

The recorder is deliberately non-blocking. If it cannot start, Unreal logs a warning and gameplay continues.

Each run writes raw evidence beneath:

```text
prototypes/rendering-benchmark/unreal/Saved/Playtests/<UTC-session-id>/
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

## F9 issue marker

During a playtest, press **F9** when something looks or feels wrong.

The recorder will:

1. append an `issue_marker` event,
2. request a screenshot into the current playtest session,
3. increment the marker count in `session.json`, and
4. show a short on-screen confirmation.

This lets the tester continue playing instead of stopping to reconstruct the issue immediately.

## Raw evidence versus repository artifacts

Do **not** commit complete `Saved/Playtests/` sessions to Git. Raw screenshots, logs, crashes, and telemetry can become large quickly.

The desktop playtest launcher may collect those files into the user's external playtest archive.

When a compact report is useful, run:

```powershell
python tools/playtest/summarize_session.py "prototypes/rendering-benchmark/unreal/Saved/Playtests/<session-id>"
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
