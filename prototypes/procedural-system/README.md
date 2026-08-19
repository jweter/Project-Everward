# Procedural System Prototype

## Purpose

Prove deterministic generation of a compact, physically plausible-enough star-system data model from stable procedural inputs without requiring untouched systems to be stored.

## Generation contract

```text
universe_seed + spatial_coordinate + generation_algorithm_version
    -> canonical StarSystem
```

The current prototype uses a SHA-256 counter stream rather than Python's built-in random generator so the reference output is explicitly tied to our own versioned algorithm rather than interpreter RNG behavior.

## Current output

- stable system and entity IDs,
- weighted stellar spectral classes,
- scaled stellar mass, radius, temperature and luminosity,
- zero to ten planets with ordered semi-major axes,
- rocky, oceanic, icy, dwarf, gas-giant and ice-giant body classes,
- generated moons with ordered local orbits,
- zero to two asteroid/minor-body belts,
- normalized resource profiles for iron, nickel, silicates, water, carbon, volatiles, radioactives and rare metals,
- canonical JSON serialization,
- SHA-256 system fingerprints.

Most canonical quantities are represented as scaled integers. This is deliberate: deterministic simulation data should not depend on platform-specific floating-point serialization where it can be avoided.

## Stable identities

Entity IDs derive from the generation root plus a structural path such as:

```text
system
star
planet/3
planet/3/moon/2
belt/1
```

The generated identity therefore remains stable for a fixed seed, coordinate and generator version.

## Resource representation

Resource composition is represented in basis points and normalized to exactly 10,000 basis points (100%). Different body classes bias the generated composition without treating the values as final scientific models.

The current generator is a technical proof, not the final astrophysical model. Scientific distributions will be deepened later without abandoning the versioned deterministic contract.

## Run the proof

From the repository root:

```bash
python prototypes/procedural-system/run_demo.py
python -m unittest discover -s prototypes/procedural-system -p 'test_*.py' -v
```

## Tests currently prove

- identical inputs reproduce identical canonical output and fingerprints,
- changed coordinates create distinct systems,
- generator version participates in system identity,
- stellar values remain within the declared spectral-class ranges,
- planetary and lunar orbits are strictly ordered,
- resource profiles normalize to exactly 100%,
- generated entity IDs are unique inside each system,
- asteroid belts have valid geometry,
- invalid generator versions are rejected,
- a pinned bank of golden seeds (ordinary, sparse/no-planet, resource-rich,
  rare spectral classes, interstellar-scale coordinates) reproduces its exact
  recorded canonical output, per `docs/TESTING_STRATEGY.md`'s golden-seed
  testing layer,
- every spectral class in `STAR_TABLE` is represented by at least one pinned
  golden case, so a change that reshuffles which class a given roll selects
  cannot pass silently.

## Golden-seed regression bank

`golden_seeds.json` pins the exact canonical output and fingerprint for a
small, diverse set of seed/coordinate pairs at the current
`GENERATOR_VERSION`. `test_golden_seeds.py` replays each case and fails if
`generator.py` produces different output for an existing generator version,
catching accidental algorithm drift that per-call equality checks and
range/invariant tests cannot catch on their own. The bank also pins at least
one case for every spectral class declared in `STAR_TABLE` (`M`, `K`, `G`,
`F`, `A`, `B`, `O`); `test_generator.py`'s range check can only compare a
generated value against the same `STAR_RANGES` entry the generator used to
produce it, so it cannot detect a class/range mismatch on its own — a pinned
case per class closes that blind spot.

A deliberate generation-algorithm change must bump `GENERATOR_VERSION` and
regenerate `golden_seeds.json` from the new algorithm as part of that same
change; the fixture must never be hand-edited to make a failing case pass.

## Acceptance criteria

- [x] identical inputs reproduce identical canonical output,
- [x] changed coordinates create distinct systems,
- [x] generated properties satisfy declared invariants,
- [x] stable entity identities have a reproducible derivation strategy,
- [x] untouched systems require no persistent storage,
- [ ] benchmark generation throughput at region-streaming scale,
- [ ] validate richer astrophysical distributions before promotion to production architecture.

## Production boundary

This prototype does **not** commit Everward to Python or to these exact numerical ranges. It establishes reference semantics:

> Versioned deterministic input must be sufficient to reconstruct an untouched star system exactly.

When the production engine/runtime is selected, equivalent behavior can be reimplemented and regression-tested against canonical fixtures.