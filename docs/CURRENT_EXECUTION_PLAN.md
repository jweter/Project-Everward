# Everward Current Execution Plan

This file is the durable pointer used by human and automated development to select the next player-visible Everward work.

## Authoritative detailed plan

Use [`PHASE2_VERTICAL_SLICE_PLAN.md`](PHASE2_VERTICAL_SLICE_PLAN.md) as the detailed execution sequence for late Phase 2, Phase 3, and the beginning of Phase 4.

The master phase boundaries remain defined by [`ROADMAP.md`](ROADMAP.md).

The canonical damaged-awakening, staged self-repair, and starter-zone departure experience is defined in [`STARTER_AWAKENING_AND_SELF_REPAIR.md`](STARTER_AWAKENING_AND_SELF_REPAIR.md). Earlier slices should preserve the component, damage, resource, power, manipulator, and repair architecture required to assemble that opening without a later rewrite.

## Selection rule

When choosing the next Everward development task:

1. inspect current `main`, open PRs, CI, and the latest Product Reality evidence;
2. repair failing or higher-priority open work first;
3. keep the earliest incomplete slice as the completion/release priority, but do not turn a pending local Product Reality check into a repository-wide stop when a later sub-slice qualifies for the explicit parallel-safe lane in `PHASE2_VERTICAL_SLICE_PLAN.md`;
4. otherwise choose the earliest incomplete slice in `PHASE2_VERTICAL_SLICE_PLAN.md` whose prerequisites are satisfied;
5. split a slice again if it cannot be implemented, reviewed, and locally Product-Reality-tested as one small PR;
6. prefer player-visible progress over unrelated infrastructure;
7. do not jump to later phases merely because a later task is easier to implement in isolation.

Parallel-safe work may merge after green portable CI only when it does not assume, hide, or alter the behavior still awaiting Product Reality, remains reversible, preserves the simulation/adapter ownership boundary, and is explicitly recorded as **implemented, Product Reality pending**. It does not complete the slice or advance a phase/release gate.

## Current near-term sequence

1. scan payoff and control legibility;
2. persistent subsystem consequences;
3. collision/contact foundation;
4. impact severity and damage foundation;
5. Prime Generation-1 probe body blockout;
6. articulated manipulator arms;
7. object selection and physical interaction;
8. dedicated zero-g space environment;
9. planetary-body foundation;
10. surface/near-surface operations;
11. deeper science gameplay;
12. resource/sample loop;
13. processing/fabrication;
14. repair/recoverability, including authoritative component integrity and staged Self Repair priorities;
15. assemble the canonical starter awakening: damaged surface start -> reachable mining -> staged recovery -> every required system at 100% -> first departure.

The canonical Self Repair planner must prioritize **restore capability before restoring perfection**: preserve survival, restore important offline systems to minimum useful operation, bring all required systems online, then reassess and repair the weakest/highest-value systems in stages toward 100%.

Everward is explicitly both a space game and a planetary-environment game. Each completed slice should make the next tested build visibly more like inhabiting and operating a real autonomous interstellar machine.
