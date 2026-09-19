# Save Format

Everward campaigns may span hundreds of hours and many game versions. Save compatibility must therefore be treated as a versioned data-contract problem, not as engine-object serialization.

## Current v2 implementation boundary

The current vertical-slice implementation is intentionally narrower than the eventual campaign schema below. The engine-independent save-data layer uses an explicit `save_version`, validates loads fail-closed, and has an ordered migration boundary (`apply_ordered_save_migrations` / `registered_save_migrations` in `save_data.hpp`).

Version 1 persisted the canonical probe/runtime state: a campaign of multiple probes (`SaveGameV1.probes`, each keyed by a stable, non-empty, unique `probe_id`) and, additively, a top-level `lineages` list recording `probe_id` / `lineage_id` / `parent_probe_id` / `generation` facts (`probe_lineage.hpp`'s `ProbeLineageRecord`), fail-closed validated on load against this same save's persisted `probe_id`s so a record can never reference a probe that was not actually persisted. `lineages` is a data-only persistence contract, not a successor mechanic: nothing in the simulation today creates a second probe or a lineage record during play, so every campaign save currently produced by the game has an empty `lineages` list.

Version 2 (issue #270, the decomposed generated-world persistence remainder of #167) adds a top-level `generated_regions` list: for each procedurally-identified region a player has actually observed or modified, a stable `region_id`/`coordinate` plus that region's own observation/modification delta only (`generated_region.hpp`'s `GeneratedRegionRecord`), never the region's full deterministically-regenerable baseline content. `generated_region.hpp` also implements the first concrete `Generate(universe_seed, spatial_coordinate, algorithm_version)` function (`generate_region_baseline`, `generation_algorithm_version` 1) and the `BaseRegion + PersistedObservations + PersistedModifications` recombination (`reconstruct_region_entities`) the "Procedural region persistence" section below describes; `validate_generated_regions` fails closed on a `region_id` inconsistent with its own coordinate or an observation/modification naming a body no generator run would ever produce. This is, like `lineages`, a data-only persistence contract: no gameplay system yet creates, explores, or presents a generated region, and the coordinate/scale mapping into in-simulation meters remains open pending the spatial-model prototype. Unlike every v1 field above, `generated_regions` is not absent-tolerant -- a save captured before it existed has no reading of "what regions were ever generated," so migrating it is v1 -> v2's first real (non-empty) use of the migration framework (`migrate_v1_generated_regions_to_v2`), covered by fixture tests in `save_data_tests.cpp`.

The following remain future schema-expansion work rather than implicit v1/v2 guarantees: `ActiveEntities` (autonomous agents or other probes actually present in a region); the authoritative successor/generation-change mechanic that would ever populate `lineages` with more than a single root record; campaign history; and the other top-level campaign categories listed below. Those additions must introduce stable IDs and generation-algorithm versioning where applicable and must not be inferred from transient Unreal objects.

This boundary keeps the implemented vertical-slice save contract truthful while preserving the long-duration campaign requirements in this document.

## Required principles

1. Every save declares a `save_version`.
2. Procedural content declares the relevant generation algorithm version.
3. Persistent entities use stable IDs.
4. Generated-but-unmodified space should be reproducible rather than exhaustively stored.
5. Observations, modifications, autonomous state, history, and player-created structures are persisted.
6. Migrations are explicit, testable, and ordered.
7. Unknown or unsupported save versions fail clearly rather than loading partially corrupted state.

## Conceptual top-level schema

```text
save_version
build_version
created_at
last_saved_at
universe_seed
generation_algorithm_versions
simulation_time
player
settings
difficulty
generated_regions
modified_regions
astronomical_entities
probes
probe_designs
lineages
infrastructure
resource_stores
messages
discoveries
research
technologies
software_policies
doctrines
civilizations
historical_events
migration_metadata
```

Not every category is required in the first prototype. The top-level shape should evolve through explicit schema versions.

## Player state

Persistent player-continuity data should eventually include:

- current consciousness host probe ID,
- legacy bodies,
- backups/recovery state where technologically available,
- player-authored doctrines,
- UI/preferences that belong in the campaign,
- major decision history where relevant.

## Probe state

A probe record should be data-oriented and reference stable definitions where possible. Likely fields include:

- probe ID,
- design ID,
- lineage ID,
- parent ID,
- generation,
- creation time,
- location/trajectory state,
- mass and component state,
- energy/thermal state,
- storage,
- software configuration,
- local knowledge,
- directives/doctrine,
- behavior parameters,
- active tasks,
- communication state,
- health/damage state.

## Procedural region persistence

Conceptual rule:

```text
BaseRegion = Generate(seed, coordinate, algorithm_version)
LoadedRegion = BaseRegion + PersistedObservations + PersistedModifications + ActiveEntities
```

Do not store a copy of the entire unmodified generated universe merely because it was once visible on a map.

Persist what can no longer be reconstructed exactly from deterministic generation or what the player is entitled to know because it has been observed.

`generated_region.hpp` implements this rule for `generation_algorithm_version` 1 (see "Current v2 implementation boundary" above): `generate_region_baseline` is `Generate(...)`, `GeneratedRegionRecord` is the persisted `PersistedObservations`/`PersistedModifications` delta, and `reconstruct_region_entities` recombines them. `ActiveEntities` has no implementation yet, since no autonomous-agent or multi-probe-presence mechanic exists in the simulation.

## Historical events

Important campaign events should be durable records with stable IDs and timestamps. Examples:

- probe created,
- consciousness transferred,
- system discovered,
- major scan result,
- facility constructed,
- interstellar departure/arrival,
- message sent/received,
- lineage independence,
- first contact,
- major research breakthrough,
- destruction/recovery event.

The history ledger can later support UI, debugging, analytics, achievements, narrative summaries, and player-facing civilization history.

## Migration model

Use ordered migrations:

```text
v1 → v2 → v3 → current
```

A migration should:

1. validate expected source version,
2. transform data deterministically,
3. record the migration,
4. validate target invariants,
5. never silently discard unknown critical state.

Migration code should be covered by fixture-based tests.

## Autosave and rollback

Save mechanics and in-world consciousness backup are separate systems.

Standard save-game functionality remains available regardless of diegetic backup technology. Difficulty may alter autosave frequency, recovery convenience, or campaign consequences, but Everward does not need to become a roguelike.

## Serialization choice

Exact serialization technology is OPEN until the architecture/engine decision. Selection criteria include:

- schema evolution,
- human inspectability for debugging,
- forward/backward compatibility strategy,
- performance,
- save size,
- deterministic ordering/canonicalization where needed,
- tooling support,
- cross-platform behavior.
