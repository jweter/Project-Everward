"""The offline Aura report importer is read-only, contacts nothing, and never leaks secrets (owner-only Aura policy)."""

from __future__ import annotations

import ast
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "playtest"))

import aura_report_import as importer  # noqa: E402

SECRETISH_REPORT = {
    "run": "20261108-084846",
    "project": "Everward",
    "prompt": "Explore the game for bugs (10 minutes of play)",
    "outcome": "explored",
    "summary": "Saved under C:\\Users\\jerem\\AppData\\Local\\Autopilot; contact owner@example.com",
    "episodes": 3,
    "seconds": 760.1,
    "tool_seconds": 596.3,
    "agent": "claude",
    "model": "claude-opus-5-5",
    "turns": 46,
    "cost_usd": 4.445815880000001,
    "findings": ["Arm can't be reversed mid-stow. token=abcdef0123456789 Bearer eyJhbGciOiJIUzI1NiJ9.payload.sig"],
    "evidence": ["trace2 t=12.5 api_key: sk-live_ABCDEFGHIJKLMNOP"],
    "account": {"id": "acct_secret_value", "email": "owner@example.com"},
    "reading_usage": {"claude-sonnet-5-5": {"calls": 48}},
}


def make_run(root: Path, report: dict | None = SECRETISH_REPORT, findings_md: str | None = None) -> Path:
    run = root / "runs" / "20261108-084846"
    run.mkdir(parents=True)
    if report is not None:
        (run / "report.json").write_text(json.dumps(report), encoding="utf-8")
    if findings_md is not None:
        (run / "findings.md").write_text(findings_md, encoding="utf-8")
    return run


class AuraReportImportTests(unittest.TestCase):
    def test_exports_only_whitelisted_fields_and_redacts_secrets(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            run = make_run(Path(tmp), findings_md="## Finding 1 (from episode 1, 08:54:59)\nbSurveyed went true->false\n")
            out = importer.import_run(run)
        text = json.dumps(out)
        for leaked in ("jerem", "owner@example.com", "abcdef0123456789", "sk-live", "eyJhbGciOiJIUzI1NiJ9", "acct_secret"):
            self.assertNotIn(leaked, text, leaked)
        self.assertNotIn("account", out)                     # not whitelisted
        self.assertNotIn("reading_usage", out)
        self.assertEqual(out["estimated_cost_usd"], 4.45)    # an estimate, labelled as such
        self.assertEqual(out["episodes"], 3)
        self.assertEqual(out["run"], "20261108-084846")
        self.assertTrue(out["findings"][0].startswith("Arm can't be reversed mid-stow."))
        self.assertEqual(out["background_findings_md"][0]["heading"], "Finding 1 (from episode 1, 08:54:59)")
        self.assertEqual(len(out["source"]["report_sha256"]), 64)

    def test_source_paths_survive_and_random_blobs_do_not(self) -> None:
        kept = "see codex/source/Everward/EverwardPlayerControllerMiningControls.cpp:164"
        self.assertEqual(importer.sanitize(kept), kept)
        token = "aGVsbG8gd29ybGQ1234567890abcdefXYZ0987654321qwerty"
        self.assertEqual(importer.sanitize(f"key {token}"), "key <redacted-blob>")

    def test_a_missing_report_is_refused_not_invented(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            run = make_run(Path(tmp), report=None)
            with self.assertRaises(FileNotFoundError):
                importer.import_run(run)

    def test_it_never_writes_inside_the_owners_run_directory(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            run = make_run(Path(tmp))
            with self.assertRaises(SystemExit):
                importer.main(["--run-dir", str(run), "--out", str(run / "sanitized.json")])
            self.assertFalse((run / "sanitized.json").exists())
            target = Path(tmp) / "out" / "sanitized.json"
            self.assertEqual(importer.main(["--run-dir", str(run), "--out", str(target)]), 0)
            self.assertTrue(target.is_file())

    def test_the_importer_cannot_reach_a_network_or_start_a_process(self) -> None:
        tree = ast.parse((ROOT / "tools" / "playtest" / "aura_report_import.py").read_text(encoding="utf-8"))
        imported = {alias.name.split(".")[0] for node in ast.walk(tree) if isinstance(node, ast.Import) for alias in node.names}
        imported |= {node.module.split(".")[0] for node in ast.walk(tree) if isinstance(node, ast.ImportFrom) and node.module}
        forbidden = {"socket", "subprocess", "urllib", "http", "requests", "asyncio", "ssl", "ftplib", "smtplib", "webbrowser", "multiprocessing"}
        self.assertEqual(imported & forbidden, set())
        calls = {node.func.attr for node in ast.walk(tree) if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)}
        self.assertFalse(calls & {"system", "popen", "startfile", "spawnv", "execv"})


if __name__ == "__main__":
    unittest.main()
