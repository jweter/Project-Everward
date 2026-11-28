"""Contract tests for seed-forwarded exploratory Godot playtesting and truthful classification."""
from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from run_playtest import DEFAULT_MODES, run_mode


class SeededExploreRunnerTests(unittest.TestCase):
    def test_exploration_is_part_of_default_unattended_suite(self):
        self.assertIn("exploretest", DEFAULT_MODES)

    def _fake_engine(self, verdict: str = "PASS", checks=None):
        commands = []

        def run(cmd, **_kwargs):
            commands.append(cmd)
            outarg = next(item for item in cmd if item.startswith("--out="))
            path = Path(outarg.split("=", 1)[1])
            path.mkdir(parents=True, exist_ok=True)
            report = {
                "verdict": verdict,
                "checks": checks or [{"name": "conservation", "ok": True, "detail": "exact"}],
                "preconditions": [{"name": "seeded setup", "ok": True, "detail": ""}],
                "reasons": [] if verdict == "PASS" else ["invariant"],
                "build": {"commit": "test-head", "dirty": False},
                "engine": {"version": "Godot test double"},
            }
            (path / "report.json").write_text(json.dumps(report), encoding="utf-8")
            return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")

        return commands, run

    def test_replay_seed_and_bounded_steps_are_passed_to_real_game_mode(self):
        commands, runner = self._fake_engine()
        with tempfile.TemporaryDirectory() as directory, patch("run_playtest.free_ram_gb", return_value=8.0):
            with patch("run_playtest.subprocess.run", side_effect=runner):
                result = run_mode("godot-test", "exploretest", Path(directory), False, 23, 15, 2.0,
                                  ["--steps=12"])
        self.assertEqual("PASS", result["classification"])
        self.assertEqual(1, len(commands))
        command = commands[0]
        self.assertIn("--headless", command)
        self.assertIn("--exploretest", command)
        self.assertIn("--seed=23", command)
        self.assertIn("--steps=12", command)

    def test_failed_exploratory_invariant_does_not_get_marked_pass(self):
        commands, runner = self._fake_engine(
            verdict="FAIL", checks=[{"name": "seed 23 action 02 conserves state", "ok": False,
                                     "detail": "injected negative control"}])
        with tempfile.TemporaryDirectory() as directory, patch("run_playtest.free_ram_gb", return_value=8.0):
            with patch("run_playtest.subprocess.run", side_effect=runner):
                result = run_mode("godot-test", "exploretest", Path(directory), False, 23, 15, 2.0,
                                  ["--steps=12", "--selftest-fail=explore-invariant"])
        self.assertEqual("FAIL", result["classification"])
        self.assertEqual(1, len(result["failed"]))
        self.assertEqual("seed 23 action 02 conserves state", result["failed"][0]["name"])


if __name__ == "__main__":
    unittest.main()
