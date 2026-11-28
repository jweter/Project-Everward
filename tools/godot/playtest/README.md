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

Headless runs record the visual status `NOT_RUN`; only `--rendered` renders. `--rendered` runs `visualtest`, whose checks read the frames themselves, and a first loop whose frames are only captured (REVIEW_REQUIRED). `visualtest` reports VERIFIED only when those checks passed (artistic quality still unjudged); when they failed, the run was inconclusive, or the engine log has script errors, the visual status says so instead — it never claims a verification the checks rejected. With no renderer `visualtest` is INCONCLUSIVE, so a visual pass is never reported without a frame. Exit code 0 for PASS or
REVIEW_REQUIRED overall, 1 for FAIL, 2 for ENVIRONMENT_FAILURE.

Modes (each also runnable alone, see `tools/godot/world-spike/README.md`): `firstloop` (damaged awakening, scan,
survey, approach, both arms and a rotated probe, mine, cargo, Fix_It repair with material and energy, conservation,
HUD), `powertest` (G09 power policy drop and recovery), `hudtest` (G11 Systems panel), `approachtest` (G10 obstacles,
turns, moving targets), `contacttest` (G07 compound hull, moving bodies, impact damage), **`exploretest`** (seeded real-input sequences, per-step inventory/repair/energy/integrity invariants, and genuine player-command recovery to a successful mine), and **`controltest`** (every control the game binds, pressed in the order a player learns it, each with the effect it must have -- including the held flight keys, the boost, the brake, the mouse, the drill and the hollow, which no other mode exercises at all). `--mode-arg` passes an
argument to every mode, e.g. `--mode-arg=--selftest-fail=noclear` to prove a FAIL is reported.

`controltest` answers a different question from the rest: not "does the loop work" but "does each control still do its job". The other modes press controls incidentally, so a control that quietly broke would show up late or not at all. It is also the only mode that runs with manual flight live, because a scripted run turns flight off and a flight control cannot be tested with flight off.

Seeded exploration runs by default with the six-mode headless suite. Use `--seed 23 --explore-steps 20` to generate a deterministic 20-action trace and the same playable recovery challenge on each run. The report contains the input sequence, simulation ticks, resource/repair totals and failure reasons; replay with the same seed, steps and exact commit. `--modes exploretest --mode-arg=--selftest-fail=explore-invariant` is a **negative control** which must fail without altering mechanics or material. It tests a bounded set of currently supported real keys (scan, mining refusal, arm selection/deploy/tool, policy, sensor power, Systems panel); it does not yet claim broad, intelligent exploration or human-like perception.

**Laptop protection:** headless by default (no window, no input capture); `--rendered` is refused without
`--exclusive`; every run first needs `--min-free-ram-gb` (default 2) free or is an ENVIRONMENT_FAILURE. No network,
no paid services, no permission to merge, push, or change owner controls.

**Release builds:** the scenarios run only with explicit command-line flags in the world spike, a developer tool
that is not shipped. A shipped game must exclude `main.gd`'s test modes (planned: gate them behind a `playtest`
export feature tag).

## Next slices (planned, not built yet)
1. ~~Runner, classification, reports~~.
2. ~~Machine-verified rendered checks~~: `--visualtest` compares drawn-vs-simulated arm geometry, probe and arm
   placement, HUD fit and simulation agreement, frame variety, frame rate and memory on real rendered frames.
3. ~~Seeded exploratory player foundation~~: bounded random real-input sequences, interrupted mining and recovery,
   conservation, no-contact, HUD and replay trace are implemented in `--exploretest`. **Still pending:** save/load
   mid-operation, moving-target exploration and a persisted failing-seed regression corpus.
4. Optional local vision review through Ollama, advisory only, if it runs acceptably on Azathoth.
5. Iron God: run the headless suite unattended under worker authority and post `playtest.json` summaries through the
   approved mailbox; repair handoffs cite the failing run's reproduction command.
