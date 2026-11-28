#!/usr/bin/env python3
"""Regression tests for the playtest runner's classification (`python test_run_playtest.py`, no dependencies).

The runner is the thing that decides whether a playtest is believed, so the way it can be wrong is by reporting a
result the evidence does not support: a pass over a crashed run, a pass over last week's frames, a clean suite over
no scenarios, or -- the other direction -- an environment failure over a run that did everything right. Each case
below is one of those, reproduced through the real command line with a stand-in for the Godot executable that writes
exactly the evidence a run of that shape leaves behind.

The stand-in writes evidence; it never decides a classification. What is under test is run_playtest.py's reading of
that evidence, which is the only part of the pipeline these tests can honestly cover: they say nothing about whether
the game works or whether a frame looks right.
"""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_playtest.py"

# Writes a run's evidence into --out: PNGs of a believable size, then a report.json, then the exit code asked for.
STUB = '''
import json, sys
from pathlib import Path
args = sys.argv[1:]
cfg = json.loads(Path(sys.argv[0]).with_name("stub.json").read_text())
out = Path(next(a.split("=", 1)[1] for a in args if a.startswith("--out=")))
mode = next(a[2:] for a in args if a.startswith("--") and a[2:] in cfg["modes"])
spec = cfg["modes"][mode]
out.mkdir(parents=True, exist_ok=True)
for i in range(spec["frames"]):
    (out / ("%s_%02d.png" % (mode, i))).write_bytes(b"\\x89PNG" + b"\\0" * 40000)
if spec.get("report", True):
    (out / "report.json").write_text(json.dumps({
        "schema": "everward.godot.evidence/1", "verdict": spec.get("verdict", "PASS"),
        "reasons": spec.get("reasons", []), "build": {"commit": "stub", "known": True, "dirty": False},
        "engine": {"version": "4.7.2.stub"},
        "checks": [{"name": "check %d" % i, "ok": i not in spec.get("failed", []), "detail": "stub"}
                   for i in range(spec.get("checks", 9))],
        "preconditions": [{"name": "a renderer is present", "ok": True, "detail": "stub"}]}, indent=2))
print(spec.get("stdout", mode + " stub"))
sys.exit(spec.get("exit", 0))
'''


def make_stub(d: Path, modes: dict) -> Path:
    """A fake Godot: `modes` maps a mode name to {frames, verdict, exit, checks, failed, report, stdout}."""
    (d / "stub.py").write_text(STUB, encoding="utf-8")
    (d / "stub.json").write_text(json.dumps({"modes": modes}), encoding="utf-8")
    stub = d / "stub.cmd" if sys.platform == "win32" else d / "stub.sh"
    if sys.platform == "win32":
        stub.write_text('@echo off\r\n"%s" "%%~dp0stub.py" %%*\r\n' % sys.executable, encoding="utf-8")
    else:
        stub.write_text('#!/bin/sh\nexec "%s" "$(dirname "$0")/stub.py" "$@"\n' % sys.executable, encoding="utf-8")
        stub.chmod(0o755)
    return stub


def run(stub: Path, out: Path, *extra: str) -> tuple[int, str, dict | None]:
    p = subprocess.run([sys.executable, str(RUNNER), "--godot", str(stub), "--out", str(out), *extra],
                       capture_output=True, text=True, errors="replace")
    summary = out / "playtest.json"
    return p.returncode, p.stdout + p.stderr, json.loads(summary.read_text()) if summary.exists() else None


def result(summary: dict, mode: str) -> dict:
    return next(r for r in summary["runs"] if r["mode"] == mode)


CASES = []


def case(fn):
    CASES.append(fn)
    return fn


@case
def a_successful_visual_run_is_a_pass(d: Path) -> None:
    """--visualtest saves three frames by design; a flat five-frame minimum called every good run an environment
    failure, so the suite could never report the visual checks it had actually done (Codex #458)."""
    stub = make_stub(d, {"visualtest": {"frames": 3}, "firstloop": {"frames": 9}})
    code, _, summary = run(stub, d / "out", "--modes=,", "--rendered", "--exclusive")
    visual = result(summary, "visualtest")
    assert visual["classification"] == "PASS", visual
    assert visual["visual"].startswith("VERIFIED"), visual["visual"]
    assert len(visual["frames"]) == 3, visual["frames"]
    assert code == 0, code


@case
def a_short_visual_run_is_an_environment_failure(d: Path) -> None:
    """The minimum still has to bite: two frames means the run stopped early, whatever its report says."""
    stub = make_stub(d, {"visualtest": {"frames": 2}})
    code, _, summary = run(stub, d / "out", "--modes=,", "--rendered", "--exclusive")
    visual = result(summary, "visualtest")
    assert visual["classification"] == "ENVIRONMENT_FAILURE", visual
    assert "2" in visual["reason"] and "3" in visual["reason"], visual["reason"]
    assert code == 2, code


@case
def a_short_first_loop_is_an_environment_failure(d: Path) -> None:
    """--firstloop photographs nine moments; four means it did not finish the loop."""
    stub = make_stub(d, {"visualtest": {"frames": 3}, "firstloop": {"frames": 4}})
    code, _, summary = run(stub, d / "out", "--modes=,", "--rendered", "--exclusive")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "ENVIRONMENT_FAILURE", loop
    assert code == 2, code


@case
def a_failed_visual_check_never_reads_as_verified(d: Path) -> None:
    """VERIFIED has to mean the visual checks passed on real frames. A run whose arm-geometry check failed has
    inspected frames too, and must report what the inspection found (Codex #458)."""
    stub = make_stub(d, {"visualtest": {"frames": 3, "verdict": "FAIL", "failed": [2],
                                        "reasons": ["the drawn arm matches the simulated arm"], "exit": 14}})
    code, _, summary = run(stub, d / "out", "--modes=,", "--rendered", "--exclusive")
    visual = result(summary, "visualtest")
    assert visual["classification"] == "FAIL", visual
    assert "VERIFIED" not in visual["visual"], visual["visual"]
    assert visual["visual"].startswith("FAILED"), visual["visual"]
    assert code == 1, code


@case
def an_inconclusive_visual_run_never_reads_as_verified(d: Path) -> None:
    """Nor may a run that could not establish its own situation claim verification."""
    stub = make_stub(d, {"visualtest": {"frames": 3, "verdict": "INCONCLUSIVE",
                                        "reasons": ["build identity unknown"], "exit": 11}})
    _, _, summary = run(stub, d / "out", "--modes=,", "--rendered", "--exclusive")
    visual = result(summary, "visualtest")
    assert visual["classification"] == "ENVIRONMENT_FAILURE", visual
    assert "VERIFIED" not in visual["visual"], visual["visual"]
    assert visual["visual"].startswith("INCONCLUSIVE"), visual["visual"]


@case
def script_errors_hold_back_a_visual_verification(d: Path) -> None:
    """Checks can pass while the engine log is full of script errors; that is for a person, not a VERIFIED stamp."""
    stub = make_stub(d, {"visualtest": {"frames": 3, "stdout": "SCRIPT ERROR: res://main.gd:1 - boom"}})
    _, _, summary = run(stub, d / "out", "--modes=,", "--rendered", "--exclusive")
    visual = result(summary, "visualtest")
    assert visual["classification"] == "REVIEW_REQUIRED", visual
    assert "VERIFIED" not in visual["visual"], visual["visual"]


@case
def captured_but_unjudged_frames_need_review(d: Path) -> None:
    """Only --visualtest inspects frames. A rendered --firstloop captures them for a person, so its frames are never
    a visual pass on their own -- the owner's rule that no visual result is reported without inspection."""
    stub = make_stub(d, {"visualtest": {"frames": 3}, "firstloop": {"frames": 9}})
    _, _, summary = run(stub, d / "out", "--modes=,", "--rendered", "--exclusive")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "REVIEW_REQUIRED", loop
    assert loop["visual"].startswith("REVIEW_REQUIRED"), loop["visual"]


@case
def a_headless_run_never_claims_a_visual_result(d: Path) -> None:
    stub = make_stub(d, {"firstloop": {"frames": 0}})
    code, _, summary = run(stub, d / "out", "--modes=firstloop")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "PASS", loop
    assert loop["visual"].startswith("NOT_RUN"), loop["visual"]
    assert code == 0, code


@case
def a_crash_after_a_passing_report_is_a_failure(d: Path) -> None:
    """The report is written before shutdown, so a PASS report plus an abnormal exit is a finding (Codex #456)."""
    stub = make_stub(d, {"firstloop": {"frames": 0, "exit": 139}})
    code, _, summary = run(stub, d / "out", "--modes=firstloop")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "FAIL", loop
    assert "139" in loop["reason"], loop["reason"]
    assert code == 1, code


@case
def a_failed_check_is_a_failure(d: Path) -> None:
    stub = make_stub(d, {"firstloop": {"frames": 0, "verdict": "FAIL", "failed": [2], "reasons": ["check 2"]}})
    code, _, summary = run(stub, d / "out", "--modes=firstloop")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "FAIL", loop
    assert len(loop["failed"]) == 1, loop["failed"]
    assert code == 1, code


@case
def an_inconclusive_report_is_an_environment_failure(d: Path) -> None:
    """INCONCLUSIVE means the run could not establish its own situation: never a finding about the game."""
    stub = make_stub(d, {"firstloop": {"frames": 0, "verdict": "INCONCLUSIVE", "reasons": ["build identity unknown"]}})
    code, _, summary = run(stub, d / "out", "--modes=firstloop")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "ENVIRONMENT_FAILURE", loop
    assert "build identity unknown" in loop["reason"], loop["reason"]
    assert code == 2, code


@case
def stale_evidence_is_never_inherited(d: Path) -> None:
    """A run that writes nothing must not be scored on the previous run's report (Codex #456)."""
    stub = make_stub(d, {"firstloop": {"frames": 0}})
    out = d / "out"
    (out / "firstloop").mkdir(parents=True)
    (out / "firstloop" / "report.json").write_text('{"verdict": "PASS", "checks": []}', encoding="utf-8")
    code, _, summary = run(stub, out, "--modes=firstloop")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "ENVIRONMENT_FAILURE", loop
    assert "already holds evidence" in loop["reason"], loop["reason"]
    assert code == 2, code


@case
def no_scenarios_is_not_a_clean_suite(d: Path) -> None:
    """An empty automation parameter must never read as 'everything passed' (Codex #456)."""
    stub = make_stub(d, {"firstloop": {"frames": 0}})
    code, log, _ = run(stub, d / "out", "--modes=,")
    assert code == 2, code
    assert "no scenarios selected" in log, log


@case
def rendered_needs_exclusive_use_of_the_machine(d: Path) -> None:
    """Rendered runs open a window; the owner's laptop is not to be taken over without saying so."""
    stub = make_stub(d, {"visualtest": {"frames": 3}})
    code, log, _ = run(stub, d / "out", "--modes=,", "--rendered")
    assert code == 2, code
    assert "refused" in log, log


@case
def an_unlaunchable_executable_is_an_environment_failure(d: Path) -> None:
    """A --godot that exists but will not run is a setup problem, not a traceback (Codex #456)."""
    bogus = d / "not-an-exe.bin"
    bogus.write_bytes(b"\x00\x01\x02")
    code, _, summary = run(bogus, d / "out", "--modes=firstloop")
    loop = result(summary, "firstloop")
    assert loop["classification"] == "ENVIRONMENT_FAILURE", loop
    assert "could not launch" in loop["reason"], loop["reason"]
    assert code == 2, code


def main() -> int:
    failed = []
    for fn in CASES:
        with tempfile.TemporaryDirectory(prefix="playtest-test-") as tmp:
            try:
                fn(Path(tmp))
                print(f"  ok   {fn.__name__}")
            except AssertionError as e:
                failed.append(fn.__name__)
                print(f"  FAIL {fn.__name__}: {e}")
    print(f"\n{len(CASES) - len(failed)}/{len(CASES)} runner classification tests passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
