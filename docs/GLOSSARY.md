# Glossary

This glossary establishes project terminology so design and implementation discussions remain consistent.

## Core terms

**Everward** — Working title of the project and game.

**Probe** — A physically embodied autonomous machine intelligence capable of sensing, acting, manufacturing, and potentially reproducing.

**Player consciousness** — The machine intelligence currently controlled by the player. It may remain in its present body or transfer into a newly constructed successor.

**Body** — The physical hardware hosting a machine intelligence at a given time.

**Successor** — A newly designed and manufactured probe architecture derived from an existing lineage.

**Consciousness transfer** — Instantiation mode in which player continuity moves into the successor body and the former body continues as an autonomous legacy machine under instructions.

**Independent child** — A newly instantiated autonomous intelligence that inherits selected knowledge, code, parameters, and directives while the player remains in the current body.

**Legacy machine** — A previous player body or other older probe that continues autonomously after consciousness transfer.

**Lineage** — The ancestry/descendant structure connecting probe intelligences across replications.

**Generation** — A position in a lineage relative to replication events. Generation number is genealogical, not a conventional character level.

**Doctrine** — A reusable set of priorities, constraints, goals, and behavior policies used by autonomous descendants.

**Directive** — A high-level inherited objective or constraint.

**Behavior parameter** — A continuous or discrete value influencing autonomous decisions, such as risk tolerance or curiosity.

**Behavior Profile** — The versioned structured data describing a probe's directives, priorities, traits, constraints, communication style, quirks, stress responses, and generator provenance.

**Behavior Generator** — The player-assistance system that creates editable Behavior Profiles from deterministic seeds, presets, constraints, locked traits, and optional natural-language intent.

**Quirk** — A lower-priority preference, routine, habit, or expressive tendency that makes a descendant memorable without normally overriding mission-critical directives.

**Behavior seed** — Deterministic seed used with generator version and constraints to reproduce a generated Behavior Profile.

**Generator version** — Version identifier for Behavior Generator logic so generated profiles can remain reproducible across software changes.

**Locked trait** — A player-selected behavior element preserved while other profile sections are regenerated.

## Simulation terms

**Simulation truth** — Authoritative mechanical state determined by deterministic game systems.

**Presentation** — Rendering, HUD, audio, narration, animation, and other systems that communicate simulation truth to the player.

**Simulation time** — Authoritative campaign time independent of rendered frame rate.

**Time acceleration** — Player-controlled increase in the rate at which simulation time advances.

**Scheduled event** — A future state transition queued for a specific simulation time.

**Headless simulation** — Running the authoritative simulation without graphics or interactive presentation.

**Universe seed** — Top-level deterministic seed used as an input to procedural generation.

**Generation algorithm version** — Version identifier included in procedural-generation inputs so saved campaigns remain reproducible across algorithm changes.

**Region** — A spatial subdivision generated on demand from deterministic inputs.

**Persistent region** — A generated region whose observations, modifications, or active entities require stored state.

**Event ledger** — Historical record of important simulation events.

## Gameplay terms

**Industrial bootstrap** — The process of using the probe's initial capabilities to establish self-sustaining mining, energy, refining, storage, and fabrication capacity.

**Observation-driven research** — Scientific progression unlocked or accelerated by observing phenomena, operating systems, experimenting, and discovering materials rather than spending generic science points alone.

**Engineering budget** — Candidate model for limiting successor changes according to available mass, materials, energy, fabrication precision, technology, computation, and infrastructure.

**Communication latency** — Physical delay between transmission and receipt of information over distance.

**Local knowledge** — The information currently available to a probe at its location, which may differ from the player's newer or older knowledge elsewhere.

**Serenity / Explorer / Voyager / Survivor / Abyss** — The five planned default difficulty presets, from exploration-focused safety to unbounded high-threat challenge.

## Scope terms

**Prototype 0** — Smallest gameplay proof: one star, one probe, one asteroid/resource problem, industrial bootstrap, and one successor.

**Vertical slice** — A polished 30–60 minute representative experience covering awakening through first successor and first interstellar step.

**Production engine** — The engine selected after evidence from Unreal/Godot technical proofs. It is intentionally undecided during Phase 0/1.
