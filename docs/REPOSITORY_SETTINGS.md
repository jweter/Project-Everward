# Repository Settings Baseline

This document records the intended GitHub repository configuration for Everward. It is not a substitute for the live GitHub settings; review this file whenever repository ownership, collaboration model, CI, or release workflow changes.

## Current posture

Everward is a private, proprietary, pre-production commercial project.

Recommended repository metadata:

- Visibility: Private
- Default branch: `main`
- Issues: Enabled
- Discussions: Disabled until there is a real collaboration/community need
- Projects: Optional; use only if it improves roadmap execution
- Wiki: Disabled; project documentation belongs in version-controlled `docs/`

## Pull-request merge policy

Recommended:

- Allow squash merge: **Enabled**
- Allow merge commits: **Disabled** after the current foundation history if a linear history policy is adopted
- Allow rebase merge: **Disabled** unless there is a specific workflow need
- Automatically delete head branches: **Enabled**
- Always suggest updating pull request branches: **Enabled**
- Auto-merge: Optional after CI is reliable and branch rules are active

Rationale: Everward is expected to generate many small, focused engineering and design PRs. Squash merging preserves one coherent main-branch commit per reviewed change and makes later reverts easier.

## `main` protection / ruleset

Once GitHub Actions can execute reliably, protect `main` with a branch ruleset or branch-protection rule.

Recommended rules:

- Require a pull request before merging.
- Require status checks to pass before merging.
- Require the `repository-constitution` check initially; add real test/lint/benchmark gates as they are introduced.
- Require conversation resolution before merging.
- Block force pushes.
- Block branch deletion.
- Prefer linear history if squash-only merging is adopted.
- Do **not** require an approving review while the repository has only one active developer; that would create a self-review deadlock. Add review requirements when a second authorized contributor exists.
- Do **not** require signed commits until the local development and automation signing strategy is intentionally established.

Important: do not make a CI job required while the GitHub account is unable to start Actions jobs. A billing/spending-limit infrastructure failure would otherwise block every merge even though no project test ran.

## Actions

Recommended Actions permissions for now:

- Default workflow token permissions: read repository contents unless a workflow explicitly requires more.
- Do not allow workflows to create/approve pull requests unless an actual automation needs that capability.
- Pin third-party Actions to trusted major releases initially; for release/security-sensitive workflows, consider pinning immutable commit SHAs after the workflow stabilizes.
- Keep job names unique if they become required checks.

## Security

Recommended as available for the account/repository plan:

- Dependency graph: Enabled once dependency manifests exist.
- Dependabot alerts: Enable once meaningful third-party dependencies appear.
- Dependabot security updates: Enable after the dependency/update workflow is proven.
- Secret scanning / push protection: Enable when available for the repository plan.
- Private vulnerability reporting: Consider before external collaborators or public distribution.

See `SECURITY.md`.

## Large files and Git LFS

Do not place large production binaries directly into ordinary Git history.

The repository includes `.gitattributes` now for line-ending and binary handling, but deliberately does not enable broad Git LFS patterns before the engine and asset pipeline are chosen.

After the production engine decision:

1. Install/configure Git LFS for every development machine and CI environment that needs assets.
2. Track only the large binary file types actually used by the selected pipeline.
3. Commit the resulting `.gitattributes` changes before adding those assets.
4. Keep engine-generated caches/build products ignored rather than placing them in LFS.

## Releases and environments

Do not create production release environments yet.

Before external builds, establish separate development/test/release configuration and protect release secrets. Steam/store signing and credentials must never live in the repository.

## Ownership and continuity

`CODEOWNERS` currently assigns the repository to `@jweter`.

Before adding outside contributors, define:

- contributor IP assignment/license terms appropriate to the commercial project,
- confidentiality expectations where needed,
- access level,
- asset provenance requirements,
- review responsibility,
- offboarding/revocation procedure.

A public contribution workflow should not be enabled accidentally while the project remains proprietary.

## Review cadence

Revisit this document at these gates:

- production engine selection,
- first executable prototype,
- first external collaborator,
- first external playtest build,
- Steam/store integration,
- public repository or source-availability decision,
- Early Access / 1.0 release preparation.
