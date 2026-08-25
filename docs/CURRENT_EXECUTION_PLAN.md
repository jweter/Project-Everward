# Everward Current Execution Plan

This file is the durable pointer used by human and automated development to select the next player-visible Everward work.

## Authoritative detailed plan

Use [`PHASE2_VERTICAL_SLICE_PLAN.md`](PHASE2_VERTICAL_SLICE_PLAN.md) as the detailed execution sequence for late Phase 2, Phase 3, and the beginning of Phase 4.

The master phase boundaries remain defined by [`ROADMAP.md`](ROADMAP.md).

## Selection rule

When choosing the next Everward development task:

1. inspect current `main`, open PRs, CI, and the latest Product Reality evidence;
2. repair failing or higher-priority open work first;
3. finish the current vertical slice before advancing;
4. otherwise choose the earliest incomplete slice in `PHASE2_VERTICAL_SLICE_PLAN.md` whose prerequisites are satisfied;
5. split a slice again if it cannot be implemented, reviewed, and locally Product-Reality-tested as one small PR;
6. prefer player-visible progress over unrelated infrastructure;
7. do not jump to later phases merely because a later task is easier to implement in isolation.

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
14. repair/recoverability.

Everward is explicitly both a space game and a planetary-environment game. Each completed slice should make the next tested build visibly more like inhabiting and operating a real autonomous interstellar machine.
