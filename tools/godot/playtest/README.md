# Everward Godot playtest runner (developer only)

One command plays the game's acceptance scenarios through the real Godot world spike (real key handler, the
simulation decides everything, nothing is edited to make a check pass) and writes one report.

```
python tools/godot/playtest/run_playtest.py --godot <godot-voxel editor exe>        # headless, all modes
python tools/godot/playtest/run_playtest.py --godot <exe> --rendered --exclusive   # plus a windowed run with frames
```

Output (default `C:/IronGod/TestTemp/playtest/<UTC>/`): `playtest.json` (schema `everward.godot.playtest/1`),
`playtest.md`, `playtest.html`, and per run its `report.json` (exact commit, dirty state, extension MD5, engine,
timed events, checks; see `tools/godot/world-spike/evidence.gd`), `metrics.txt`, `engine.log` and any frames.

| Result | Meaning |
|---|---|
| PASS | the mode's report says PASS and the engine log has no script errors |
| FAIL | a check failed while every precondition held, or every check passed but the game exited abnormally (a crash on the way out is still a finding): the report lists the failed checks and the exact command to reproduce |
| REVIEW_REQUIRED | mechanics passed but a person must look: script errors, or rendered frames (never machine-passed) |
| ENVIRONMENT_FAILURE | the run could not establish its situation: report INCONCLUSIVE, no fresh report, timeout, too little RAM, or a rendered run that saved fewer frames than its mode takes (`visualtest` 3, `firstloop` 5 of its 9) |

The runner decides whether a playtest is believed, so its own reading of the evidence is tested:
`python tools/godot/playtest/test_run_playtest.py` (no dependencies, seconds, no Godot) drives the real command line
against a stand-in executable that writes the evidence of a crashed run, a stale directory, a short rendered run, and
so on, and asserts the classification. Every runner defect found in review has a case here.

Evidence is never shared between runs: only a `report.json` and frames written after the run started count, and a run directory that already holds evidence stops that run instead of mixing the two.

Headless runs record the visual status `NOT_RUN`; only `--rendered` renders. `--rendered` runs `visualtest`, whose checks read the frames themselves (visual status VERIFIED, artistic quality still unjudged), and a first loop whose frames are only captured (REVIEW_REQUIRED). With no renderer `visualtest` is INCONCLUSIVE, so a visual pass is never reported without a frame. Exit code 0 for PASS or
REVIEW_REQUIRED overall, 1 for FAIL, 2 for ENVIRONMENT_FAILURE.

Modes (each also runnable alone, see `tools/godot/world-spike/README.md`): `firstloop` (damaged awakening, scan,
survey, approach, both arms and a rotated probe, mine, cargo, Fix_It repair with material and energy, conservation,
HUD), `powertest` (G09 power policy drop and recovery), `hudtest` (G11 Systems panel), `approachtest` (G10 obstacles,
turns, moving targets), `contacttest` (G07 compound hull, moving bodies, impact damage). `--mode-arg` passes an
argument to every mode, e.g. `--mode-arg=--selftest-fail=noclear` to prove a FAIL is reported.

**Laptop protection:** headless by default (no window, no input capture); `--rendered` is refused without
`--exclusive`; every run first needs `--min-free-ram-gb` (default 2) free or is an ENVIRONMENT_FAILURE. No network,
no paid services, no permission to merge, push, or change owner controls.

**Release builds:** the scenarios run only with explicit command-line flags in the world spike, a developer tool
that is not shipped. A shipped game must exclude `main.gd`'s test modes (planned: gate them behind a `playtest`
export feature tag).

## Next slices (planned, not built yet)
1. ~~Runner, classification, reports~~.
2. ~~Machine-verified rendered checks: drawn-vs-simulated arm geometry, probe and arm on screen, HUD fit and
   agreement with the simulation, frame variety, frame rate and memory~~ (`--visualtest`).
3. Seeded exploratory player: random but reproducible key sequences (invalid actions, interrupted operations,
   save/load mid-operation) with invariants (no stuck state past a timeout, conservation, no contact during
   approach, HUD agrees with the simulation); a failing seed becomes a regression case.
4. Optional local vision review through Ollama, advisory only, if it runs acceptably on Azathoth.
5. Iron God: run the headless suite unattended under worker authority and post `playtest.json` summaries through the
   approved mailbox; repair handoffs cite the failing run's reproduction command.
