# Contributing to Everward

Everward is currently a private commercial project in pre-production. Contributions should preserve the project constitution and keep architecture decisions evidence-based.

## Before changing behavior

Read:

1. `docs/VISION.md`
2. `docs/DESIGN_PILLARS.md`
3. `docs/SIMULATION_PHILOSOPHY.md`
4. `docs/ROADMAP.md`
5. `docs/TECHNOLOGY_DECISIONS.md`
6. `docs/DECISION_LOG.md`

If a proposed change conflicts with a governing design pillar or accepted ADR, update the decision record explicitly rather than bypassing it in code.

## Branch and PR workflow

- `main` is the integration branch.
- Develop meaningful changes on focused branches.
- Open a pull request rather than pushing substantial work directly to `main`.
- Keep PRs small enough to review and test coherently.
- Include tests for simulation behavior when executable code exists.
- Do not merge failing CI.
- Record major architecture/design choices in `docs/DECISION_LOG.md`.

Suggested branch prefixes:

- `feat/`
- `fix/`
- `docs/`
- `test/`
- `perf/`
- `prototype/`
- `chore/`

## Quality preflight

Use the repository-standard preflight before opening or updating a PR:

```bash
python tools/quality_preflight.py
```

That fast mode validates the repository constitution and Git diff hygiene. For simulation, prototype, foundation-tooling, or release-sensitive changes, run full CI parity locally:

```bash
python tools/quality_preflight.py --full
```

Full mode additionally configures/builds/tests the production C++ simulation core with CMake/CTest and runs every Python unittest suite currently enforced by the Foundation workflow. CI remains authoritative; do not suppress a failing invariant merely to make the preflight pass.

## Coding principles

- Simulation truth must not depend on presentation.
- Simulation time must not depend on rendered frame rate.
- Prefer explicit state and stable IDs over hidden engine-object coupling.
- Deterministic systems must use controlled random streams.
- Persist schemas intentionally; do not blindly serialize runtime objects.
- Avoid premature abstractions for late-game systems not yet required by the roadmap.
- Do not optimize away testability.

## Testing expectations

For simulation changes, add the smallest test that proves the intended invariant or behavior.

Prioritize:

- deterministic unit tests,
- headless integration tests,
- save/load equivalence,
- fixed-seed regressions,
- invariant checks,
- performance measurements for scaling-sensitive changes.

See `docs/TESTING_STRATEGY.md`.

## Documentation expectations

Documentation is part of the architecture.

Update the relevant docs when a change affects:

- design pillars,
- terminology,
- roadmap sequencing,
- architecture boundaries,
- save schema,
- technology decisions,
- IP/license obligations,
- performance assumptions.

## Assets and dependencies

Do not add a third-party asset or dependency without known provenance and commercial-use terms.

Update `docs/IP_AND_LICENSES.md` or the future asset/dependency manifest with the source and applicable license.

## Scope discipline

The first playable proof does **not** require aliens, civilizations, warfare, megastructures, infinite-tech progression, multiplayer, millions of active systems, a complete programming language, a full soundtrack, or Steam integration.

When in doubt, favor work that advances the next success gate rather than the most distant feature.
