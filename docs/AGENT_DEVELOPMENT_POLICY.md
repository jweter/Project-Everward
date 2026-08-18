# Agent Development Policy

This document governs scheduled and autonomous engineering work on Everward. It exists to keep progress high-value, reviewable, truthful, reversible, and aligned with the project's design and commercial constraints.

## 1. Project isolation

Everward is its own engineering and product context. Do not import product requirements, terminology, architecture, safety rules, roadmap priorities, implementation choices, music-processing requirements, or unrelated project assumptions from other repositories merely because they appeared in another conversation or project.

Reload Everward context from verified repository state and its authoritative documentation before making substantive changes.

## 2. Source-of-truth hierarchy

When guidance conflicts, use this order and surface unresolved contradictions rather than silently guessing:

1. non-negotiable vision and design pillars;
2. accepted decisions and ADRs in `DECISION_LOG.md`;
3. current roadmap phase, milestone, and exit gate;
4. architecture, testing, security, IP/licensing, save, and performance constraints;
5. issue and PR acceptance criteria;
6. implementation details and local convenience.

`PROJECT_STATUS.md` records the current operational continuation point but does not override the hierarchy above.

## 3. Hourly run triage

Every scheduled development run begins by inspecting verified repository state before starting new roadmap work. Inspect as necessary:

- open pull requests;
- current check/CI status;
- mergeability;
- unresolved blocking review threads or requested changes;
- recent relevant issues;
- `PROJECT_STATUS.md`;
- current roadmap phase and exit gate;
- relevant architecture/design/decision/testing/security/IP documents.

Never invent repository state.

## 4. PR state machine

Existing work takes priority over new work. Classify each relevant open PR as GREEN, FAILED, PENDING, CONFLICTED, BLOCKED, or UNCERTAIN.

### GREEN

A PR may be merged automatically only when all of the following are true:

- every required check applicable to the PR is present;
- all required checks have succeeded;
- mergeability is resolved and mergeable;
- required review conditions are satisfied;
- no unresolved blocking review comments remain;
- no material correctness, architecture, security, licensing, IP/provenance, save-compatibility, performance, or project-policy concern remains;
- repository policy permits automated merging.

A newly opened PR does not require waiting for a later hourly run if fresh independent GitHub CI has already completed successfully and every merge condition above is satisfied. Local tests alone never qualify a PR as GREEN.

After merging, verify that the merge actually succeeded.

### FAILED

If CI or another required gate fails:

1. inspect the actual failing check or log;
2. identify the first meaningful failing gate;
3. determine the likely root cause from evidence;
4. reproduce locally when appropriate and possible;
5. make the smallest safe correction on the existing PR branch;
6. add or update regression protection when appropriate;
7. run targeted tests/checks;
8. push the correction to the existing PR;
9. allow fresh CI to evaluate the new head;
10. do not claim the failure is fixed until evidence supports that conclusion.

For significant, recurring, or instructive failures, update `ERROR_RESOLUTION_LEDGER.md` with symptom, failing gate, root cause, evidence, fix, verification, regression protection, and residual risk.

### PENDING

Do not merge. Do not duplicate the work. Do not create dependent work that assumes the pending change has merged unless repository policy explicitly permits it.

### CONFLICTED / BLOCKED / UNCERTAIN

Investigate and resolve routine engineering conflicts when clearly safe. Never force merge merely to create progress. Escalate only when a genuine human decision boundary is reached.

## 5. New roadmap development

Only begin new roadmap work when no higher-priority existing work requires action.

For each run, perform at most one substantial new implementation slice for Everward.

The slice must:

1. come from the current authorized roadmap phase;
2. prefer completing the current milestone/exit gate before later phases;
3. represent the highest-value currently authorized work;
4. be small, coherent, and reviewable;
5. avoid unrelated opportunistic changes;
6. include appropriate tests/validation;
7. update affected documentation before it becomes stale;
8. use a focused branch and pull request;
9. avoid direct substantive commits to `main`.

Prefer finishing work over starting work.

## 6. Priority order

Use this order unless an explicit repository decision states otherwise:

- **P0** — security issue, corruption/data-loss risk, critical save breakage, major correctness regression, or critical safety/IP problem;
- **P1** — existing PR with failing CI that can be safely repaired;
- **P2** — existing PR verified GREEN and ready to merge;
- **P3** — blocking review feedback or merge conflict;
- **P4** — incomplete current milestone or previously started work;
- **P5** — highest-value authorized roadmap development;
- **P6** — refactoring, optimization, documentation cleanup, or technical debt without an immediate blocker.

Within the same priority: unblock dependent work first, address older blockers, favor higher-value product work, and prefer the smallest safe coherent implementation.

## 7. Documentation freshness

Documentation is part of the implementation contract, not optional cleanup.

Whenever a change materially affects project truth, update the relevant authoritative documents in the same PR when practical. This includes, as applicable:

- `ROADMAP.md`;
- `PROJECT_STATUS.md`;
- `ARCHITECTURE.md`;
- `DECISION_LOG.md`;
- `TESTING_STRATEGY.md`;
- `SAVE_FORMAT.md`;
- `PERFORMANCE_BUDGETS.md`;
- `IP_AND_LICENSES.md`;
- `REPOSITORY_SETTINGS.md`;
- `ERROR_RESOLUTION_LEDGER.md`;
- feature-specific design documents.

Do not allow documentation to remain knowingly stale after a change merges.

## 8. Operational memory

`PROJECT_STATUS.md` is the durable continuation record for automation. Update it whenever relevant work changes the current phase, active milestone, active PR, completed milestone evidence, blockers, or exact next continuation point.

Use `DECISION_LOG.md` for durable why/architecture/product decisions and `ERROR_RESOLUTION_LEDGER.md` for significant failure memory. Do not overload `PROJECT_STATUS.md` with historical detail that belongs in those records.

## 9. CI and verification

"Green" means all required checks applicable to the current PR, not merely the original foundation workflow. As unit, integration, determinism, lint, save-migration, benchmark, engine, or other required gates are introduced, they automatically become part of the merge standard when configured as required checks.

Never substitute local success for independent GitHub verification when required CI exists.

Never merge code with red, pending, missing-required-check, conflicted, or materially uncertain status.

## 10. Human decision boundaries

Routine implementation and engineering judgments should be made autonomously when supported by project documentation and evidence.

Escalate to Jeremy for decisions such as:

- fundamental product direction;
- a major architecture change not authorized by roadmap/docs;
- a new paid service or meaningful recurring cost;
- license/IP ownership posture changes;
- destructive data/save migrations;
- privacy/security boundary changes;
- credential/secret problems requiring owner action;
- public release, publishing, store, or commercialization decisions requiring authorization;
- irreversible operations;
- genuinely ambiguous requirements where materially different product outcomes are possible.

Do not interrupt for routine coding judgments.

## 11. Public repository, proprietary IP

The repository is intentionally public for operational reasons, but Everward remains proprietary and all-rights-reserved unless an explicit later decision changes that posture.

Public visibility is not an open-source license grant. Do not introduce language implying that public access grants permission to copy, modify, redistribute, commercialize, or create derivative works from Everward's original materials.

Third-party components remain governed by their own licenses and attribution requirements.

## 12. Truthfulness and safety

Never fabricate repository access, files, tests, CI results, issues, commits, PR numbers, merge results, benchmark evidence, or project progress.

Never expose or commit secrets.

Never change unrelated files solely because they were noticed.

Prefer small, reversible, testable changes.
