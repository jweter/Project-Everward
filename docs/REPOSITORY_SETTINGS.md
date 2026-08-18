# Repository Settings Baseline

This document records the intended GitHub repository configuration for Everward. It is not a substitute for live GitHub settings; review this file whenever repository ownership, collaboration model, CI, automation, or release workflow changes.

## Current posture

Everward is a **publicly visible, proprietary, pre-production commercial project**.

Public visibility is a deliberate operational choice and is not an open-source license grant. The root `LICENSE` and `docs/IP_AND_LICENSES.md` govern Everward's original material.

Recommended repository metadata:

- Visibility: **Public**
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
- Auto-merge: Allowed only when repository rules and `AGENT_DEVELOPMENT_POLICY.md` conditions are satisfied

Everward is expected to generate many small, focused engineering and design PRs. Squash merging preserves one coherent main-branch commit per reviewed change and makes later reverts easier.

## `main` protection / ruleset

Protect `main` with a branch ruleset or branch-protection rule as CI becomes reliable.

Recommended rules:

- Require a pull request before merging.
- Require all applicable required status checks to pass before merging.
- Require the `repository-constitution` check initially; add real unit, integration, determinism, lint, migration, benchmark, engine, or other gates as they become authoritative.
- Require conversation resolution before merging.
- Block force pushes.
- Block branch deletion.
- Prefer linear history if squash-only merging is adopted.
- Do **not** require an approving review while the repository has only one active developer; that can create a self-review deadlock. Add review requirements when a second authorized contributor exists.
- Do **not** require signed commits until the local development and automation signing strategy is intentionally established.

Important: do not make a CI job required while the GitHub account is unable to start that job. Infrastructure failure must be distinguished from an actual project-test failure.

Substantive scheduled/autonomous development must use branches and pull requests rather than direct commits to `main`.

## Automated merge semantics

A scheduled agent may merge a PR without waiting for a later hourly cycle if fresh independent GitHub CI has completed and the PR is fully GREEN under `AGENT_DEVELOPMENT_POLICY.md`.

Never merge because local tests passed alone. Never merge red, pending, conflicted, missing-required-check, blocked, or materially uncertain work.

## Actions

Recommended Actions permissions for now:

- Default workflow token permissions: read repository contents unless a workflow explicitly requires more.
- Do not allow workflows to create/approve pull requests unless an actual automation needs that capability.
- Pin third-party Actions to trusted major releases initially; for release/security-sensitive workflows, consider immutable commit SHAs after stabilization.
- Keep job names unique if they become required checks.

## Security for a public proprietary repository

Recommended as available for the account/repository plan:

- Dependency graph: Enabled once dependency manifests exist.
- Dependabot alerts: Enable once meaningful third-party dependencies appear.
- Dependabot security updates: Enable after the dependency/update workflow is proven.
- Secret scanning / push protection: Enable whenever available.
- Private vulnerability reporting: Consider before outside contributors or public builds.

Never commit credentials, tokens, private keys, signing material, store credentials, personal/private data, or restricted third-party assets. See `SECURITY.md` and `IP_AND_LICENSES.md`.

## Large files and Git LFS

Do not place large production binaries directly into ordinary Git history.

The repository includes `.gitattributes` for line-ending and binary handling, but deliberately does not enable broad Git LFS patterns before the engine and asset pipeline are chosen.

After the production engine decision:

1. install/configure Git LFS for every development machine and CI environment that needs assets;
2. track only the large binary file types actually used by the selected pipeline;
3. commit the resulting `.gitattributes` changes before adding those assets;
4. keep engine-generated caches/build products ignored rather than placing them in LFS.

## Releases and environments

Do not create production release environments yet.

Before external builds, establish separate development/test/release configuration and protect release secrets. Steam/store signing and credentials must never live in the repository.

## Ownership and continuity

`CODEOWNERS` assigns the repository to `@jweter`.

Before accepting outside contributors, define:

- contributor IP assignment/license terms appropriate to the commercial project;
- confidentiality expectations where needed;
- access level;
- asset/code provenance requirements;
- review responsibility;
- offboarding/revocation procedure.

Public repository visibility must not be treated as automatic authorization for contributions or reuse.

## Automation governance

Scheduled development is governed by:

- `docs/AGENT_DEVELOPMENT_POLICY.md` — PR/CI state machine, priority, merge rules, documentation freshness, and escalation boundaries;
- `docs/PROJECT_STATUS.md` — durable current phase, blockers, and continuation point;
- `docs/ERROR_RESOLUTION_LEDGER.md` — significant failure memory;
- `docs/DECISION_LOG.md` — durable accepted/open project decisions.

## Review cadence

Revisit this document at these gates:

- major automation-policy change;
- production engine selection;
- first executable prototype;
- first external collaborator;
- first external playtest build;
- Steam/store integration;
- repository visibility or source-availability decision;
- Early Access / 1.0 release preparation.
