# Save Format

Everward campaigns may span hundreds of hours and many game versions. Save compatibility must therefore be treated as a versioned data-contract problem, not as engine-object serialization.

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
