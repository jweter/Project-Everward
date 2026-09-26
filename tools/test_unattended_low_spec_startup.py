from __future__ import annotations

import copy
import importlib.util
import json
import re
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"
LAUNCHER_PATH = ROOT / "tools" / "run_unattended_low_spec_startup.ps1"
HARNESS_PATH = ROOT / "tools" / "run_phase2_first_playtest.ps1"
PUBLISH_PATH = ROOT / "tools" / "publish_unattended_product_reality.ps1"

spec = importlib.util.spec_from_file_location("everward_unattended_worker_low_spec", WORKER_PATH)
assert spec is not None and spec.loader is not None
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)

COMMIT = "a" * 40


def passing_evidence() -> dict:
    return {
        "schema_version": 1,
        "profile": "low_spec_development",
        "git_commit": COMMIT,
        "expected_commit": COMMIT,
        "startup": {
            "status": "pass",
            "launched": True,
            "alive_through_sample_window": True,
            "exited_during_sample_window": False,
            "exit_code": None,
            "main_window_observed": True,
            "seconds_to_main_window": 21.5,
        },
        "memory": {
            "metric": "editor_peak_working_set_mib",
            "sample_window_seconds": 60,
            "sample_count": 118,
            "peak_working_set_mib": 2310.4,
            "final_working_set_mib": 2201.0,
        },
        "cleanup": {"status": "stopped", "method": "close_main_window"},
        "product_reality_claimed": False,
        "failure_reason": None,
    }


def harness_low_spec_commands() -> str:
    text = HARNESS_PATH.read_text(encoding="utf-8")
    match = re.search(r"\$LowSpecCommands = (@\([^)]*\))", text)
    assert match is not None
    return match.group(1)


class LowSpecStartupEvidenceTests(unittest.TestCase):
    def evaluate(self, evidence, *, exit_code: int = 0) -> dict:
        return worker.evaluate_low_spec_startup_evidence(
            evidence, expected_commit=COMMIT, exit_code=exit_code
        )

    def test_complete_bounded_evidence_passes_without_product_reality_claim(self) -> None:
        result = self.evaluate(passing_evidence())
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["commit"], COMMIT)
        self.assertEqual(result["memory"]["peak_working_set_mib"], 2310.4)
        self.assertEqual(result["memory"]["sample_count"], 118)
        self.assertIs(result["product_reality_claimed"], False)
        self.assertIn("not visual", result["interpretation"])

    def test_missing_or_unreadable_evidence_is_review_required(self) -> None:
        for evidence in (None, "not json", [], {"schema_version": 99}):
            with self.subTest(evidence=evidence):
                self.assertEqual(self.evaluate(evidence)["status"], "REVIEW_REQUIRED")

    def test_commit_mismatch_is_review_required(self) -> None:
        evidence = passing_evidence()
        evidence["git_commit"] = "b" * 40
        result = self.evaluate(evidence)
        self.assertEqual(result["status"], "REVIEW_REQUIRED")
        self.assertIn("exact tested commit", result["reason"])

    def test_editor_exit_during_window_is_fail(self) -> None:
        evidence = passing_evidence()
        evidence["startup"].update(
            status="fail",
            alive_through_sample_window=False,
            exited_during_sample_window=True,
            exit_code=3,
        )
        evidence["failure_reason"] = "editor_exited_during_sample_window"
        result = self.evaluate(evidence, exit_code=1)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["startup"]["editor_exit_code"], 3)

    def test_unavailable_memory_telemetry_is_review_required(self) -> None:
        for mutate in (
            lambda e: e["memory"].update(peak_working_set_mib=None),
            lambda e: e["memory"].update(peak_working_set_mib=0),
            lambda e: e["memory"].update(sample_count=0),
            lambda e: e["memory"].update(peak_working_set_mib="2310"),
            lambda e: e["memory"].update(peak_working_set_mib=float("nan")),
        ):
            evidence = passing_evidence()
            mutate(evidence)
            with self.subTest(memory=evidence["memory"]):
                result = self.evaluate(evidence)
                self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_nonzero_launcher_exit_or_unconfirmed_cleanup_never_passes(self) -> None:
        self.assertEqual(self.evaluate(passing_evidence(), exit_code=2)["status"], "REVIEW_REQUIRED")
        self.assertEqual(self.evaluate(passing_evidence(), exit_code=124)["status"], "REVIEW_REQUIRED")
        evidence = passing_evidence()
        evidence["cleanup"]["status"] = "failed"
        self.assertEqual(self.evaluate(evidence)["status"], "REVIEW_REQUIRED")

    def test_unavailable_startup_is_review_required(self) -> None:
        evidence = passing_evidence()
        evidence["startup"].update(status="unavailable", alive_through_sample_window=False, launched=False)
        evidence["failure_reason"] = "unreal_editor_already_running"
        result = self.evaluate(evidence, exit_code=2)
        self.assertEqual(result["status"], "REVIEW_REQUIRED")
        self.assertEqual(result["failure_reason"], "unreal_editor_already_running")

    def test_evidence_claiming_product_reality_is_rejected(self) -> None:
        evidence = passing_evidence()
        evidence["product_reality_claimed"] = True
        self.assertEqual(self.evaluate(evidence)["status"], "REVIEW_REQUIRED")

    def test_only_allow_listed_facts_are_copied_so_private_paths_cannot_leak(self) -> None:
        private = r"C:\Users\someone\Documents\Everward Playtest\Project-Everward"
        evidence = passing_evidence()
        evidence["unreal_log"] = private
        evidence["failure_reason"] = f"crash in {private}"
        evidence["startup"]["status"] = private
        evidence["memory"]["source"] = private
        evidence["cleanup"]["status"] = private
        evidence["launch_contract"] = {"project": private}
        for exit_code in (0, 1, 2):
            with self.subTest(exit_code=exit_code):
                result = self.evaluate(copy.deepcopy(evidence), exit_code=exit_code)
                serialized = json.dumps(result)
                self.assertNotIn("someone", serialized)
                self.assertNotIn("Everward Playtest", serialized)
                self.assertNotEqual(result["status"], "PASS")


class LowSpecStartupRunnerTests(unittest.TestCase):
    def test_runner_fails_closed_when_launcher_writes_no_evidence_and_sanitizes_tail(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp) / "Project-Everward"
            (repo / "tools").mkdir(parents=True)
            (repo / "tools" / "run_unattended_low_spec_startup.ps1").write_text("", encoding="utf-8")
            ue = Path(temp) / "UE_5.8"
            logs = Path(temp) / "state" / "logs" / "aaaaaaaaaaaa"

            def fake_run_logged(args, *, cwd, log_path, timeout_seconds):
                self.assertLessEqual(timeout_seconds, 600.0)
                self.assertIn("-ExpectedCommit", args)
                self.assertEqual(args[args.index("-ExpectedCommit") + 1], COMMIT)
                log_path.parent.mkdir(parents=True, exist_ok=True)
                log_path.write_text(f"launcher failed at {repo} using {ue}\n", encoding="utf-8")
                return 2, 3.0

            with mock.patch.object(worker, "_is_windows", return_value=True), mock.patch.object(
                worker, "run_logged", side_effect=fake_run_logged
            ):
                result = worker.run_unreal_low_spec_startup(repo, ue, logs, COMMIT)

            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertIs(result["product_reality_claimed"], False)
            self.assertNotIn(str(repo), result["failure_tail"])
            self.assertNotIn(str(ue), result["failure_tail"])
            self.assertIn("<REPO_ROOT>", result["failure_tail"])

    def test_runner_discards_stale_evidence_from_a_previous_attempt(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp) / "repo"
            (repo / "tools").mkdir(parents=True)
            (repo / "tools" / "run_unattended_low_spec_startup.ps1").write_text("", encoding="utf-8")
            logs = Path(temp) / "logs"
            logs.mkdir()
            (logs / "low-spec-startup.json").write_text(json.dumps(passing_evidence()), encoding="utf-8")

            def fake_run_logged(args, *, cwd, log_path, timeout_seconds):
                log_path.write_text("TIMEOUT\n", encoding="utf-8")
                return 124, 300.0

            with mock.patch.object(worker, "_is_windows", return_value=True), mock.patch.object(
                worker, "run_logged", side_effect=fake_run_logged
            ):
                result = worker.run_unreal_low_spec_startup(repo, Path(temp) / "ue", logs, COMMIT)
            self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_missing_launcher_or_non_windows_is_review_required(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            result = worker.run_unreal_low_spec_startup(repo, repo, repo / "logs", COMMIT)
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            (repo / "tools").mkdir()
            (repo / "tools" / "run_unattended_low_spec_startup.ps1").write_text("", encoding="utf-8")
            with mock.patch.object(worker, "_is_windows", return_value=False):
                result = worker.run_unreal_low_spec_startup(repo, repo, repo / "logs", COMMIT)
            self.assertEqual(result["status"], "REVIEW_REQUIRED")


class LowSpecCacheTests(unittest.TestCase):
    def test_cached_pass_requires_low_spec_marker_for_same_commit(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            state = Path(temp)
            (state / worker.LAST_PASS_MARKER).write_text(COMMIT + "\n", encoding="ascii")
            self.assertIsNone(worker.read_last_pass(state))
            (state / worker.LOW_SPEC_PASS_MARKER).write_text("b" * 40 + "\n", encoding="ascii")
            self.assertIsNone(worker.read_last_pass(state))
            (state / worker.LOW_SPEC_PASS_MARKER).write_text(COMMIT + "\n", encoding="ascii")
            self.assertEqual(worker.read_last_pass(state), COMMIT)


class LowSpecWorkerOrderingTests(unittest.TestCase):
    def test_low_spec_runs_only_after_headless_smoke_passes_and_before_cache(self) -> None:
        source = WORKER_PATH.read_text(encoding="utf-8")
        run_body = source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]
        validate_index = run_body.index("validate_dedicated_checkout(repo_root)")
        sync_index = run_body.index("sync_to_target(repo_root, target, state_dir)")
        preflight_index = run_body.index("run_full_preflight(")
        build_index = run_body.index("run_unreal_build(")
        smoke_index = run_body.index("run_unreal_headless_smoke(")
        gate_index = run_body.index('if headless_smoke.get("status") != "PASS":')
        low_spec_index = run_body.index("run_unreal_low_spec_startup(")
        marker_index = run_body.index("LOW_SPEC_PASS_MARKER).write_text")
        self.assertLess(validate_index, sync_index)
        self.assertLess(sync_index, preflight_index)
        self.assertLess(preflight_index, build_index)
        self.assertLess(build_index, smoke_index)
        self.assertLess(smoke_index, gate_index)
        self.assertLess(gate_index, low_spec_index)
        self.assertLess(low_spec_index, marker_index)
        self.assertIn('report["checks"]["unreal_low_spec_startup"] = low_spec', run_body)

    def test_human_only_debt_keeps_frame_time_and_gameplay(self) -> None:
        source = WORKER_PATH.read_text(encoding="utf-8")
        self.assertIn("actual frame-time/playability until automated Unreal telemetry exists", source)
        self.assertIn("control and movement feel", source)

    def test_publisher_lists_low_spec_check_with_fingerprint_only(self) -> None:
        text = PUBLISH_PATH.read_text(encoding="utf-8")
        self.assertIn('"unreal_low_spec_startup"', text)
        self.assertNotIn("$Check.failure_tail)", text)


class LowSpecLauncherContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.launcher = LAUNCHER_PATH.read_text(encoding="utf-8")

    def test_launcher_reuses_manual_low_spec_contract_exactly(self) -> None:
        self.assertIn(f"$LowSpecCommands = {harness_low_spec_commands()}", self.launcher)
        for expected in ('"-WINDOWED"', '"-ResX=1280"', '"-ResY=720"', "WorkingSet64", "editor_peak_working_set_mib"):
            self.assertIn(expected, self.launcher)
        self.assertIn("process working set only; not total system RAM or shared-GPU memory", self.launcher)

    def test_launcher_is_dedicated_checkout_and_exact_commit_bound(self) -> None:
        self.assertIn("everward-unattended-worker", self.launcher)
        self.assertIn("dedicated_checkout_sentinel_missing", self.launcher)
        self.assertIn("commit_identity_mismatch", self.launcher)
        self.assertIn("[Parameter(Mandatory=$true)][string]$ExpectedCommit", self.launcher)

    def test_launcher_is_bounded_and_always_cleans_up_its_own_editor(self) -> None:
        self.assertIn("[Math]::Min([Math]::Max($SampleWindowSeconds, 30), 300)", self.launcher)
        self.assertIn("unreal_editor_already_running", self.launcher)
        self.assertIn('"-Unattended"', self.launcher)
        finally_body = self.launcher.split("finally {", 1)[1]
        self.assertIn("Stop-EditorTree -Process $EditorProcess", finally_body)
        self.assertIn("Write-Evidence", finally_body)
        self.assertIn("taskkill.exe /PID $Process.Id /T /F", self.launcher)
        self.assertNotIn("Stop-Process -Name", self.launcher)

    def test_launcher_does_not_touch_production_settings_or_simulation(self) -> None:
        for forbidden in ("DefaultEngine.ini", "DefaultScalability", "GameUserSettings", "Set-Content -Path $ProjectPath", "src/simulation", "src\\simulation"):
            self.assertNotIn(forbidden, self.launcher)
        self.assertNotIn("everward-manual-playtest.lock", self.launcher)

    def test_launcher_evidence_is_path_free_and_disclaims_product_reality(self) -> None:
        self.assertIn("product_reality_claimed = $false", self.launcher)
        self.assertIn("launcher_exception", self.launcher)
        evidence_block = self.launcher.split("$Evidence = [ordered]@{", 1)[1].split("function Write-Evidence", 1)[0]
        for path_variable in ("$RepoRoot", "$ProjectPath", "$UnrealRoot", "$EditorExe", "$UnrealLogPath", "$EvidencePath"):
            self.assertNotIn(path_variable, evidence_block)

    def test_manual_harness_default_launch_is_unchanged(self) -> None:
        text = HARNESS_PATH.read_text(encoding="utf-8")
        self.assertIn("[switch]$LowSpec", text)
        self.assertIn("[switch]$MeasureMemory", text)
        self.assertIn('-MeasureMemory is currently restricted to -LowSpec evidence runs.', text)


if __name__ == "__main__":
    unittest.main()
