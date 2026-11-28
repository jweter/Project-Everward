#!/usr/bin/env python3
"""Everward Godot playtest runner (developer only; #378 G04/G12/G13 and the owner's autonomous-playtest goal).

One command runs the world-spike acceptance modes through the real game (real key handler, simulation-authorized
mechanics, no state edits), collects every run's report.json, and writes one playtest report:
  <out>/playtest.json  machine-readable (schema everward.godot.playtest/1)
  <out>/playtest.md    readable summary with failed checks and exact reproduction commands
  <out>/playtest.html  the same, for a browser

Classification per run:
  PASS                 the mode's report says PASS and the engine log has no script errors
  FAIL                 a check failed while every precondition held (a finding about the game)
  REVIEW_REQUIRED      mechanics passed but something needs a person: script errors in the log, or rendered frames
                       that were captured but are only judged by a human (or an advisory vision model) -- a visual
                       result is never PASS without inspection
  ENVIRONMENT_FAILURE  the run could not establish its situation: report INCONCLUSIVE, missing or unreadable report,
                       timeout, too little free RAM, or (rendered) no frames
Visual status is recorded separately; headless runs say NOT_RUN, never PASS.

Laptop protection: headless by default (no window, no input capture). --rendered opens a game window and is refused
unless --exclusive says the worker has the machine to itself. Each run waits for --min-free-ram-gb or is classified
ENVIRONMENT_FAILURE. No network, no paid services.
"""
from __future__ import annotations

import argparse
import ctypes
import datetime as dt
import html
import json
import os
import subprocess
import sys
from pathlib import Path

SCHEMA = "everward.godot.playtest/1"
HERE = Path(__file__).resolve().parent
WORLD = HERE.parent / "world-spike"
DEFAULT_MODES = ["firstloop", "powertest", "hudtest", "approachtest", "contacttest"]
RENDERED_FRAMES_MIN = 5          # --firstloop takes 9 screenshots when a renderer exists


def free_ram_gb() -> float | None:
    if os.name != "nt":
        try:
            for line in open("/proc/meminfo"):
                if line.startswith("MemAvailable:"):
                    return int(line.split()[1]) / 1024 / 1024
        except OSError:
            return None
        return None

    class MS(ctypes.Structure):
        _fields_ = [("dwLength", ctypes.c_ulong), ("dwMemoryLoad", ctypes.c_ulong),
                    ("ullTotalPhys", ctypes.c_ulonglong), ("ullAvailPhys", ctypes.c_ulonglong),
                    ("ullTotalPageFile", ctypes.c_ulonglong), ("ullAvailPageFile", ctypes.c_ulonglong),
                    ("ullTotalVirtual", ctypes.c_ulonglong), ("ullAvailVirtual", ctypes.c_ulonglong),
                    ("ullAvailExtendedVirtual", ctypes.c_ulonglong)]
    m = MS()
    m.dwLength = ctypes.sizeof(MS)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m)):
        return None
    return m.ullAvailPhys / 1024 ** 3


def run_mode(godot: str, mode: str, out: Path, rendered: bool, seed: int, timeout: int, min_ram: float,
             extra: list[str] | None = None) -> dict:
    run_dir = out / (mode + ("-rendered" if rendered else ""))
    run_dir.mkdir(parents=True, exist_ok=True)
    # Evidence from an earlier run must never be read as this one's (Codex #456): a run that writes nothing would
    # otherwise inherit the old report.json and its old frames. Only files written after this run started count, and
    # a run directory that already holds evidence stops the run instead of mixing the two.
    stale = sorted(f.name for f in run_dir.iterdir() if f.is_file())
    if stale:
        return {"mode": mode, "rendered": rendered, "out": str(run_dir), "command": "",
                "visual": "NOT_RUN", "classification": "ENVIRONMENT_FAILURE",
                "reason": f"run directory already holds evidence ({', '.join(stale[:4])}); use a fresh --out"}
    cmd = [godot] + ([] if rendered else ["--headless"]) + ["--path", str(WORLD), "--", f"--{mode}",
                                                          f"--out={run_dir.as_posix()}", f"--seed={seed}"] + (extra or [])
    entry = {"mode": mode, "rendered": rendered, "command": subprocess.list2cmdline(cmd), "out": str(run_dir),
             "visual": "NOT_RUN (headless: no rendered frame exists)" if not rendered else "NOT_INSPECTED"}
    ram = free_ram_gb()
    entry["free_ram_gb_before"] = None if ram is None else round(ram, 2)
    if ram is not None and ram < min_ram:
        entry.update(classification="ENVIRONMENT_FAILURE", reason=f"only {ram:.1f} GB RAM free (< {min_ram} GB)")
        return entry
    started = dt.datetime.now(dt.timezone.utc)
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, errors="replace", timeout=timeout)
        log, code = p.stdout + p.stderr, p.returncode
    except subprocess.TimeoutExpired as e:
        log, code = (e.stdout or "") if isinstance(e.stdout, str) else "", None
    entry["seconds"] = round((dt.datetime.now(dt.timezone.utc) - started).total_seconds(), 1)
    entry["exit_code"] = code
    (run_dir / "engine.log").write_text(log, encoding="utf-8")
    entry["script_errors"] = log.count("SCRIPT ERROR")
    report_path = run_dir / "report.json"
    report = None
    if report_path.exists() and report_path.stat().st_mtime >= started.timestamp():
        try:
            report = json.loads(report_path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            report = None
    if code is None:
        entry.update(classification="ENVIRONMENT_FAILURE", reason=f"timed out after {timeout} s")
        return entry
    if report is None:
        entry.update(classification="ENVIRONMENT_FAILURE", reason=f"no fresh report.json (exit {code})")
        return entry
    entry["verdict"] = report.get("verdict")
    entry["reasons"] = report.get("reasons", [])
    entry["build"] = report.get("build", {})
    entry["engine"] = report.get("engine", {})
    entry["extension"] = report.get("extension", {})
    entry["memory_end"] = report.get("memory_end", {})
    entry["failed"] = [c for c in report.get("checks", []) if not c.get("ok")]
    entry["failed_preconditions"] = [c for c in report.get("preconditions", []) if not c.get("ok")]
    entry["checks_total"] = len(report.get("checks", []))
    verdict = report.get("verdict")
    if verdict == "INCONCLUSIVE":
        entry.update(classification="ENVIRONMENT_FAILURE", reason="; ".join(entry["reasons"]))
    elif verdict == "FAIL":
        entry.update(classification="FAIL", reason="; ".join(entry["reasons"]))
    elif verdict == "PASS":
        entry.update(classification="PASS", reason="")
        # A report is written before the engine shuts down, so a PASS report plus an abnormal exit means the game
        # crashed or failed on the way out: a finding, never a pass (Codex #456).
        if code != 0:
            entry.update(classification="FAIL",
                         reason=f"every check passed but the game exited abnormally (code {code})")
        elif entry["script_errors"]:
            entry.update(classification="REVIEW_REQUIRED", reason=f"{entry['script_errors']} script errors in the engine log")
    else:
        entry.update(classification="ENVIRONMENT_FAILURE", reason=f"unknown verdict {verdict!r}")
    if rendered:
        frames = sorted(str(f.name) for f in run_dir.glob("*.png")
                        if f.stat().st_size > 20000 and f.stat().st_mtime >= started.timestamp())
        entry["frames"] = frames
        if len(frames) < RENDERED_FRAMES_MIN:
            entry["visual"] = f"ENVIRONMENT_FAILURE ({len(frames)} rendered frames captured)"
            if entry["classification"] == "PASS":
                entry.update(classification="ENVIRONMENT_FAILURE", reason="rendered run produced no usable frames")
        else:
            entry["visual"] = f"REVIEW_REQUIRED ({len(frames)} frames captured; not machine-judged)"
            if entry["classification"] == "PASS":
                entry.update(classification="REVIEW_REQUIRED", reason="rendered frames need review")
    return entry


def write_reports(out: Path, summary: dict) -> None:
    (out / "playtest.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    lines = [f"# Everward playtest {summary['started_utc']}", "",
             f"Overall: **{summary['overall']}**. Commit `{summary['commit']}` (dirty: {summary['dirty']}), "
             f"Godot {summary['godot']}, seed {summary['seed']}, host {summary['host']}.", "",
             "| Run | Result | Checks | Visual | Time | Reason |", "|---|---|---|---|---|---|"]
    for r in summary["runs"]:
        name = r["mode"] + (" (rendered)" if r["rendered"] else "")
        checks = f"{r.get('checks_total', 0) - len(r.get('failed', []))}/{r.get('checks_total', 0)}"
        lines.append(f"| {name} | {r['classification']} | {checks} | {r['visual']} | {r.get('seconds', '-')} s | {r.get('reason', '')} |")
    problems = [r for r in summary["runs"] if r["classification"] != "PASS"]
    if problems:
        lines += ["", "## Needs attention", ""]
        for r in problems:
            lines += [f"### {r['mode']}{' (rendered)' if r['rendered'] else ''}: {r['classification']}", "",
                      f"- Reason: {r.get('reason', '')}", f"- Reproduce: `{r['command']}`", f"- Evidence: `{r['out']}`"]
            for c in r.get("failed_preconditions", []) + r.get("failed", []):
                lines.append(f"- FAIL {c['name']}: {c['detail'][:400]}")
            for f in r.get("frames", []):
                lines.append(f"- Frame: `{f}`")
            lines.append("")
    md = "\n".join(lines) + "\n"
    (out / "playtest.md").write_text(md, encoding="utf-8")
    body = "".join(f"<p>{html.escape(l)}</p>" if not l.startswith("|") else f"<pre>{html.escape(l)}</pre>" for l in lines)
    (out / "playtest.html").write_text(
        "<!doctype html><meta charset='utf-8'><title>Everward playtest</title>"
        "<style>body{font:14px system-ui;margin:24px;max-width:1100px}pre{margin:0;white-space:pre-wrap}</style>"
        + body, encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot", default=os.environ.get("EVERWARD_GODOT", ""), help="godot-voxel editor build")
    ap.add_argument("--out", default="", help="report directory (default C:/IronGod/TestTemp/playtest/<UTC>)")
    ap.add_argument("--modes", default=",".join(DEFAULT_MODES))
    ap.add_argument("--seed", type=int, default=1, help="recorded and passed to every mode (reproducibility)")
    ap.add_argument("--rendered", action="store_true", help="also run --firstloop in a window and capture frames")
    ap.add_argument("--exclusive", action="store_true", help="the worker has the machine to itself (required by --rendered)")
    ap.add_argument("--min-free-ram-gb", type=float, default=2.0)
    ap.add_argument("--timeout", type=int, default=900, help="seconds per run")
    ap.add_argument("--mode-arg", action="append", default=[], help="extra argument for every mode, e.g. --selftest-fail=noclear")
    a = ap.parse_args()
    if not a.godot or not Path(a.godot).exists():
        print("ENVIRONMENT_FAILURE: pass --godot or set EVERWARD_GODOT to the godot-voxel editor build")
        return 2
    if a.rendered and not a.exclusive:
        print("refused: --rendered opens a game window; add --exclusive only when nobody is using the machine")
        return 2
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    out = Path(a.out or f"C:/IronGod/TestTemp/playtest/{stamp}")
    out.mkdir(parents=True, exist_ok=True)
    runs = [run_mode(a.godot, m.strip(), out, False, a.seed, a.timeout, a.min_free_ram_gb, a.mode_arg)
            for m in a.modes.split(",") if m.strip()]
    if a.rendered:
        runs.append(run_mode(a.godot, "firstloop", out, True, a.seed, a.timeout, a.min_free_ram_gb, a.mode_arg))
    for r in runs:
        print(f"{r['classification']:<20} {r['mode']}{' (rendered)' if r['rendered'] else ''}  {r.get('reason', '')}")
    classes = {r["classification"] for r in runs}
    overall = ("FAIL" if "FAIL" in classes else "ENVIRONMENT_FAILURE" if "ENVIRONMENT_FAILURE" in classes
               else "REVIEW_REQUIRED" if "REVIEW_REQUIRED" in classes else "PASS")
    builds = [r.get("build", {}) for r in runs if r.get("build")]
    engines = [r.get("engine", {}) for r in runs if r.get("engine")]
    summary = {"schema": SCHEMA, "started_utc": stamp, "overall": overall, "seed": a.seed,
               "commit": builds[0].get("commit", "unknown") if builds else "unknown",
               "dirty": builds[0].get("dirty") if builds else None,
               "godot": engines[0].get("version", "unknown") if engines else "unknown",
               "host": f"{os.environ.get('COMPUTERNAME', 'unknown')} ({sys.platform})", "runs": runs}
    write_reports(out, summary)
    print(f"OVERALL {overall}  report: {out / 'playtest.md'}")
    return {"PASS": 0, "REVIEW_REQUIRED": 0, "FAIL": 1}.get(overall, 2)


if __name__ == "__main__":
    sys.exit(main())
