# Audio Direction

## Philosophy

Everward's audio should reinforce scale, isolation, machinery, discovery, and danger without constantly demanding attention. The baseline is atmospheric and spacious rather than relentlessly cinematic.

Silence is a valid compositional tool.

More importantly, **audio richness is part of the probe's evolving sensorium**. The player should not begin with a fully produced cinematic soundscape merely because the game can render one. Early machines perceive less. Later machines may learn to interpret, sonify, compose, sing, and develop musical traditions.

> The probe does not merely gain stronger capabilities. It gains richer ways of experiencing the universe.

## Generation-1 starting point

The canonical Generation-1 experience should be intentionally sparse.

The player may have essential interface/accessibility cues, but the in-world machine should begin close to silence:

- no continuous background score by default;
- no fake vacuum engine roar;
- only safety-critical alerts and intentionally limited internal-machine cues;
- minimal or absent sonification of contact, electromagnetic activity, or scientific data until the relevant sensing/interpretation capability exists.

This silence is not missing polish. It establishes the narrow sensory world of a primitive autonomous probe and makes later sensory growth perceptible.

Accessibility and player-safety cues are never progression-gated. Progression controls the richness of the probe's fictional perception, not whether a player can safely operate the game.

## Evolving audio perception

Audio can unlock incrementally as hardware, software, and computation improve.

A representative progression is:

```text
GEN-001
near silence / essential alerts

-> internal vibration sensing
machinery hum, relays, pumps, structural vibration

-> contact and tool sensing
impacts, scraping, drilling, manipulator force feedback

-> electromagnetic sensing
radio, plasma, magnetosphere and field sonification

-> scientific sonification
spectral, radiation, orbital and environmental data translated into sound

-> adaptive audio interpretation
context-aware ambience generated from what the machine perceives

-> generative music
probe-created background music shaped by location, activity and memory

-> vocal synthesis
humming, vocal textures, spoken or sung output

-> songwriting / musical culture
original songs, motifs, lineage themes and music exchanged between descendants
```

These are examples of reachable directions rather than a mandatory linear technology ladder. A lineage may specialize differently or ignore aesthetic audio entirely.

## Environmental sound

Audio should communicate the probe's interpreted sensorium rather than literal air-transmitted sound in vacuum. The fiction can justify sonification of telemetry, vibration, internal machinery, contact forces, electromagnetic activity, and processed scientific data.

Potential layers include:

- low mechanical drones,
- internal machinery vibration,
- mining and fabrication impacts,
- communication tones,
- sensor sweeps,
- processed radio noise,
- radiation or magnetosphere sonification,
- warning states,
- propulsion and maneuvering cues.

Each layer should have an in-world reason to exist. If the current probe lacks the sensing or interpretation capability, that perceptual layer may remain absent.

## Adaptive and generated music

Music does not need to be purely an out-of-world soundtrack. At higher capability levels, the probe itself may become capable of generating aesthetic interpretation from its experience.

That allows background music to become an earned machine capability:

- early probe: none;
- improved computation/sensing: contextual ambient synthesis;
- mature creative intelligence: original compositions;
- advanced descendants: persistent motifs, singing, songwriting, and stylistic identity.

Music intensity and character should respond to state rather than play a fixed cinematic score over every activity.

Examples:

- peaceful mining near a gas giant: sparse, slow, spacious,
- first replication: restrained but emotionally significant,
- first extrasolar arrival: expansion and awe,
- first life detection: subtle transformation,
- dangerous neutron-star approach: rising tension,
- combat: stronger rhythmic and harmonic intensity,
- unexplained signal: ambiguity rather than immediate threat coding.

## Machine creativity and lineage culture

At sufficiently advanced levels, music can become more than interface feedback.

A probe may eventually:

- compose while traveling;
- generate a theme for a discovered world;
- hum or sing while operating;
- write songs about major events or descendants;
- preserve compositions across consciousness transfer;
- send music over delayed communications;
- inherit or reject a parent's musical preferences;
- develop a distinct musical tradition within one lineage.

Different descendants may evolve different aesthetic behavior. One lineage may remain utilitarian and nearly silent. Another may treat composition as a major part of its identity. This should emerge from the same broader machinery that allows software, behavior, and culture to diverge rather than from a hard-coded "music class."

A distant child sending home a composition inspired by a star system its parent will never personally visit is a valid Everward-scale emotional moment.

## Major musical moments

The score or probe-generated music may mark rare campaign milestones such as:

- first successor,
- first interstellar departure,
- first arrival at another star,
- first independent grandchild lineage,
- first confirmed life,
- first contact,
- lineage independence,
- major scientific breakthrough,
- survival of an extreme astrophysical event.

These moments should remain rare enough to retain emotional weight.

## Simulation architecture

The engine-independent simulation owns which audio-perception capabilities the probe possesses. Unreal/audio middleware presents the resulting layers; it does not decide what the probe is capable of perceiving.

`src/simulation/include/everward/simulation/sensorium.hpp` provides the first scaffold. It evaluates an `EvolutionContext` into explicit layers including:

- essential interface cues;
- internal-machine perception;
- contact/vibration perception;
- electromagnetic sonification;
- scientific sonification;
- adaptive ambience;
- generative music;
- vocal synthesis;
- songwriting.

The first implementation is intentionally a capability contract, not a finished sound engine. Future Unreal work can bind actual assets, procedural synthesis, music systems, spatialization, and mixing to these authoritative layers without hard-coding progression into presentation.

## Implementation principle

Mechanical game state determines the context. The audio system reacts to that state; it does not determine simulation outcomes.

The long-term experiential goal is clear:

> Early Everward feels sparse because the machine can barely perceive. Late Everward can become extraordinarily rich because the player has literally evolved new senses and new ways to interpret existence.
