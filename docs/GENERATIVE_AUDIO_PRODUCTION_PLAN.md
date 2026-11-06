# Everward — Generative Audio Production Plan

**Status:** Implementation roadmap; no MetaSounds, Quartz, audio assets or Unreal runtime behavior are certified by this document.
**Design authority:** AUDIO_DIRECTION.md and existing simulation sensorium capability contract. Complements PRODUCTION_QUALITY_GATES.md. 
**Priority:** P0 for basic essential cues + mining/repair, P1 for evolved sensing and a bounded algorithmic composer, P2 for lineage culture, P3 for optional neural models.

## Creative non-negotiables

- The damaged Gen-1 Prime awakens in **deliberate near-silence**. Lack of music is a creative choice, not failed audio production.
- Essential alerts, accessibility alternatives, menu feedback and player-safety information are available from the first minute and never gated by fictional progression.
- Sounds in vacuum are *internal vibration, structural/contact transmission or deliberate sonification* of sensed data. Do not simulate ambient air-transmitted vacuum engine noise as fact.
- Richer perception must be earned by real sensing/interpretation capability, whereas music and sound settings remain fully under player control.
- The exact same authoritative contact/scan/repair event determines mechanics, animation, HUD and audio. The renderer or audio middleware cannot invent physical contact or resource change.
- Single-player gameplay must work offline; no LLM/network subscription or GPU music inference is required.

## Audio product architecture (proposed, not yet implemented)

### Source events from game truth

An immutable event envelope is the boundary between simulation and audio rendering. Proposed fields, subject to technical review:

| Field | Purpose |
|---|---|
| event_id, simulation_tick, stable_audio_seed | Event identity, reproducible variations, replay |
| source_entity_id, optional source_position | Ownership and correct spatial/perspective mix |
| action/type, material_id | Mechanical semantics and surface character |
| contact_force/load, tool_speed, damage, integrity_delta | Measurable synthesis parameters |
| sensorium_capability_flags, significance | Whether fictional perception exists and event importance |

The read-only contextual snapshot carries: sensorium tier, installed sensing channels, power/temperature/risk, current scene and isolation, discovery class, lineage motif identifier and user mix/accessibility preferences. Design the serialization/version rules before relying on these values for saves or replay.

**Required invariant:** at most the intended number of audio triggers per authoritative event ID; duplicate deliveries handled idempotently where needed; no sound event without the source game event. Synthetic QA-only triggers must remain clearly marked and cannot write simulation state.

### Rendering layers

1. Essential UI, alerts and player-accessible confirmation.
2. Internal machine: relay, motors, electrical/power and thermal equipment vibration.
3. Contact and tools: manipulator load, extraction, collection and physical impact.
4. Scientific sonification: radio, EM, plasma, radiation, spectral or orbital data, only when perceivable.
5. Optional atmospheric/algorithmic score: intensity, motifs and discovery significance.
6. Vocal synthesis/creative-machine culture: a distant, separate feature, not necessary to accept the opening.

Use an explicit gain/priority/ducking and dynamic-range policy; independent controls for music, effects, UI/alerts and sonification, plus a fully silent-musical option. Never make warnings audio-only.

### Unreal production tools

- **MetaSounds:** parameterized synthesis, layered cues, controlled event-driven variation, reusable instruments.
- **Quartz:** musically quantized transitions and event-aligned score without fragile time-based polling.
- **Audio Modulation / submix routing:** signal bus balance, mix snapshots, effect priority and fatigue controls.
- **Simulation adapter boundary:** converts immutable audio read-model into middleware parameters; no authoritative game rules in MetaSound graphs.
- **Automated capture:** logs emitted source event IDs and audio parameter changes; renders a short reproducible sound capture when supported, checks peak/loudness/fade behavior and thread budget.

Use Unreal 5.8 tools after a minimal documented proof in the real project; references to engine capabilities do not mean these assets already exist.

## Parameter mapping experiments

| Input | Synthesis / musical response | Meaning | Gate |
|---|---|---|---|
| True contact + tool load + material | transient shape, resonance, texture, controlled modulation | Touch and resistance are real | A1 |
| Extraction ticks + inventory credit | separate physical contact and successful material-acquisition cues | No imaginary ore | A1 |
| Fix_It energy/material consumption + integrity | sparse pulse → repair mechanism → newly functioning subsystem | Real restoration | A1 |
| Sensing confidence and signal type | harmonicity, filter/spectrum, spatial breadth | Information quality improved | A2 |
| Radiation/thermal risk | safe priority warning grammar and mix ducking | Player can respond | A0/A2 |
| First successor/interstellar departure | rare authored motif cells + constrained seeded variation | Life-changing transition | A3 |
| Lineage relationship + delayed report | inherit and mutate motif, stamp send/receive events | Autonomous descendants share history | A4 |

Sounds must not reveal materials or threats the probe has not detected, unless needed for accessibility or an intentional game-design exception documented separately.

## Production slices

### A0 — Reliable audio contract and near-silent opening (P0; phase-compatible)

Deliver: event/parameter registry, source IDs, silence policy, settings and accessibility defaults, audio buses, event replay fixture and safety-alert parity. Prototype one essential warning and one benign menu sound. No score required.

Pass only when a pinned Unreal build produces the approved sparse baseline, essential messages have equivalent visual/text channels, repeated input produces no phantom events, and volume/mute settings survive reload. In CI verify source-event identity and deterministic data model; actual auditory quality requires captured output and review.

### A1 — Tactile mining and Fix_It (P0; same active first-loop path)

Deliver: real tool-tip/contact sound with material/force variation, genuine storage credit, error/insufficient-power feedback, repair start/progress/complete sound, sensible spatial/internal filtering and source-log correlation.

Pass only when replayed mining and repair actions in actual Unreal produce the expected distinct event chains, sounds are bounded in level, they stop on interruption, no emission plays from a false reach/contact, and the cues remain comprehensible after repeated exposure. Visual or text alternatives remain present.

### A2 — Audio as evolved perception (P1)

Deliver: internal vibration, contact, electromagnetic and scientific sonification unlocks aligned with existing sensorium.hpp capability flags and available scientific measurements; optional player mixing settings.

Pass: identical seed and encounter before/after legitimate capability restoration shows a richer perceivable signal **without changing simulation truth**. Sonification must not produce fake discovered facts.

### A3 — Algorithmic adaptive composer (P1, exploration that cannot block A0/A1)

Deliver: authored motifs/harmonic grammar, deterministic seeded variation, scene-state transitions, sparsity, Quartz timing, music ducking, pause/time-acceleration/save behavior, opt-out, and CPU/memory budget.

Pass: distinct coherent musical results over a fixed seed suite; no stuck loops or hard cuts, comprehensible first-replication/departure cue, effective mute, sane 30+ minute fatigue evaluation and no gameplay/audio-thread regressions. Full score is **not** required in the first playable.

### A4 — Lineage music and historical memory (P2 / after successor)

Deliver: persistent motif identity and variation, successor inheritance/independence choices, delayed musical messages, replayable provenance and save migrations. Preserve machines that opt out of music.

Pass: multi-generation save/reload and delayed event replay preserve heritage without unbounded asset growth or instant knowledge leakage.

### A5 — Optional neural music/voice research (P3)

Only after A0–A3 meet quality gates, compare local/offline generation with algorithmic baseline for audio quality, licensing/rights, load/latency, CPU/RAM/GPU and thermal cost, user controls, abuse risk and any platform disclosure requirements. No paid inference or cloud dependency is approved by this plan. Stop if benefits are not demonstrable.

## Minimum required tests

- **Unit/contract:** event deduplication, capability gating, seed/replay semantics, invalid/NaN parameter clamps, no gameplay-state writes.
- **Unattended Unreal:** component creation/launch, signal present when event true, absent when event false, no stuck sound after interruption, save/reload settings, low-spec startup + memory/audio-thread telemetry where possible.
- **Perception/accessibility:** silent game remains playable, warning visual equivalence, independent mixers, predictable dynamic range and volume.
- **Human-only artistic:** musical coherency, fatigue, awe, honest silence, dramaturgy and onboarding. Do not put the owner in repetitive technical QA.
- **Provenance:** track every source recording, musical motif, sample library, generated stem and third-party plug-in in established commercial license/provenance ledger.

## Explicit dependency order

A0 source contract and safety baseline → A1 first tool-contact and repair feedback → A2 unlockable sensory channels → A3 deterministic adaptive composer → A4 lineage motifs → A5 optional model R&D.

A0/A1 can be built as **independent, reversible, phase-compatible** work around the earliest incomplete Phase-2 loop. Do not claim gameplay acceptance until the same actual Unreal run proves the corresponding game mechanic. A2/A3 prototype work must not displace scan/mine/Fix_It restore or the first cohesive one-probe experience.

## Decisions pending a small technical experiment

- Exact placement/versioning of audio source events in engine-independent simulation event bus.
- Whether MetaSound graph generation vs a thin synthesized source/custom DSP best meets Prime low-spec device budgets.
- Quantization/time-acceleration policy and whether background musical time tracks player time or simulation clock.
- Save behavior for motif seeds/composer state, with explicit distinctions between deterministic intent and rendered audio bit-for-bit fidelity.
- Objective audio loudness/headroom targets established from captured hardware runs, not invented constants.

