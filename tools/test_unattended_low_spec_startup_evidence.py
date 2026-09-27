from __future__ import annotations

import copy
import importlib.util
import json
import re
import tempfile
import unittest
from pathlib import Path
from typing import Any
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"
PROBE_PATH = ROOT / "tools" / "run_unattended_low_spec_startup.ps1"
HARNESS_PATH = ROOT / "tools" / "run_phase2_first_playtest.ps1"
PUBLISH_PATH = ROOT / "tools" / "publish_unattended_product_reality.ps1"

spec = importlib.util.spec_from_file_location("everward_unattended_worker_low_spec", WORKER_PATH)
assert spec is not None and spec.loader is not None
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)

COMMIT = "a" * 40


def low_spec_commands(source: str) -> list[str]:
    match = re.search(r"\$LowSpecCommand\w* = @\(([^)]*)\)", source)
    assert match is not None, "low-spec command list not found"
    return re.findall(r'"([^"]+)"', match.group(1))


def good_evidence() -> dict[str, Any]:
    return {
        "schema_version": 1,
        "kind": "everward_unattended_low_spec_startup",
        "git_commit": COMMIT,
        "profile": "low_spec_development",
        "resolution": "1280x720",
        "launched": True,
        "startup": {
            "status": "initialized",
            "engine_initialized_marker_observed": True,
            "seconds_to_engine_initialized": 42.5,
            "exit_code": None,
            "timeout_seconds": 900,
        },
        "memory": {
            "metric": "editor_peak_working_set_mib",
            "value": 2345.6,
            "sample_window_seconds": 60,
            "sample_interval_ms": 500,
            "sample_count": 118,
        },
        "cleanup": {"attempted": True, "process_tree_terminated": True},
        "error": "",
    }


class LowSpecStartupClassificationTests(unittest.TestCase):
    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self.temp = Path(self._temp.name)
        self.repo = self.temp / "Project-Everward"
        self.ue = self.temp / "UE_5.8"
        self.log = self.temp / "low-spec-startup.log"
        self.unreal_log = self.temp / "low-spec-startup-unreal.log"

    def summarize(self, evidence: dict[str, Any] | None, exit_code: int = 0) -> dict[str, Any]:
        return worker.summarize_low_spec_startup(
            evidence,
            exit_code=exit_code,
            duration_seconds=75.0,
            log_path=self.log,
            unreal_log_path=self.unreal_log,
            tested_commit=COMMIT,
            repo_root=self.repo,
            unreal_root=self.ue,
        )

    def test_complete_exact_commit_evidence_passes_without_claiming_product_reality(self) -> None:
        check = self.summarize(good_evidence())
        self.assertEqual(check["status"], "PASS")
        self.assertEqual(check["commit"], COMMIT)
        self.assertEqual(check["profile"], "low_spec_development")
        self.assertEqual(check["memory"]["value"], 2345.6)
        self.assertEqual(check["memory"]["sample_count"], 118)
        self.assertEqual(check["startup"]["seconds_to_engine_initialized"], 42.5)
        self.assertIs(check["product_reality_claimed"], False)
        self.assertIn("no visual, control-feel, frame-time, or gameplay", check["scope"])

    def test_missing_evidence_is_review_required(self) -> None:
        check = self.summarize(None, exit_code=0)
        self.assertEqual(check["status"], "REVIEW_REQUIRED")

    def test_commit_mismatch_is_review_required(self) -> None:
        evidence = good_evidence()
        evidence["git_commit"] = "b" * 40
        self.assertEqual(self.summarize(evidence)["status"], "REVIEW_REQUIRED")

    def test_non_low_spec_profile_is_review_required(self) -> None:
        evidence = good_evidence()
        evidence["profile"] = "default"
        self.assertEqual(self.summarize(evidence)["status"], "REVIEW_REQUIRED")

    def test_unavailable_memory_telemetry_is_review_required(self) -> None:
        for value, samples in ((None, 0), (0, 5), (1024.0, 0), (True, 3), (float("nan"), 3)):
            evidence = good_evidence()
            evidence["memory"]["value"] = value
            evidence["memory"]["sample_count"] = samples
            with self.subTest(value=value, samples=samples):
                check = self.summarize(evidence)
                self.assertEqual(check["status"], "REVIEW_REQUIRED")
                self.assertNotEqual(check["status"], "PASS")

    def test_editor_exit_before_evidence_complete_is_fail(self) -> None:
        evidence = good_evidence()
        evidence["startup"]["status"] = "exited_before_evidence_complete"
        evidence["startup"]["exit_code"] = 3
        self.unreal_log.write_text("Fatal error\n", encoding="utf-8")
        check = self.summarize(evidence, exit_code=1)
        self.assertEqual(check["status"], "FAIL")
        self.assertEqual(check["startup"]["editor_exit_code"], 3)
        self.assertIn("Fatal error", check["failure_tail"])

    def test_startup_timeout_is_inconclusive_not_pass(self) -> None:
        evidence = good_evidence()
        evidence["startup"]["status"] = "timeout_before_engine_initialized"
        evidence["startup"]["engine_initialized_marker_observed"] = False
        self.assertEqual(self.summarize(evidence, exit_code=2)["status"], "REVIEW_REQUIRED")

    def test_unconfirmed_process_cleanup_is_review_required(self) -> None:
        evidence = good_evidence()
        evidence["cleanup"]["process_tree_terminated"] = False
        self.assertEqual(self.summarize(evidence)["status"], "REVIEW_REQUIRED")

    def test_script_exit_disagreement_is_review_required(self) -> None:
        self.assertEqual(self.summarize(good_evidence(), exit_code=124)["status"], "REVIEW_REQUIRED")

    def test_not_launched_reason_is_sanitized(self) -> None:
        evidence = good_evidence()
        evidence["launched"] = False
        evidence["error"] = f"cannot open {self.ue}\\Engine and {self.repo}\\unreal under {Path.home()}"
        check = self.summarize(evidence, exit_code=3)
        self.assertEqual(check["status"], "REVIEW_REQUIRED")
        self.assertNotIn(str(self.repo), check["reason"])
        self.assertNotIn(str(self.ue), check["reason"])
        self.assertNotIn(str(Path.home()), check["reason"])
        self.assertIn("<UE_5_8_ROOT>", check["reason"])

    def test_failure_tail_is_sanitized(self) -> None:
        evidence = good_evidence()
        evidence["startup"]["status"] = "exited_before_evidence_complete"
        self.unreal_log.write_text(
            f"LogWindows: Error: crash in {self.repo}\\unreal\\Binaries\nloaded {self.ue}\\Engine\nhome {Path.home()}\n",
            encoding="utf-8",
        )
        tail = self.summarize(evidence, exit_code=1)["failure_tail"]
        self.assertNotIn(str(self.repo), tail)
        self.assertNotIn(str(self.ue), tail)
        self.assertNotIn(str(Path.home()), tail)
        self.assertIn("<REPO_ROOT>", tail)

    def test_unexpected_evidence_fields_are_not_copied_into_report(self) -> None:
        evidence = good_evidence()
        evidence["unreal_log_path"] = str(self.repo / "private.log")
        evidence["memory"]["source_path"] = str(Path.home())
        check = self.summarize(evidence)
        rendered = json.dumps(check)
        self.assertNotIn("private.log", rendered)
        self.assertNotIn("unreal_log_path", rendered)
        self.assertNotIn("source_path", rendered)

    def test_bom_encoded_evidence_is_loaded(self) -> None:
        path = self.temp / "evidence.json"
        path.write_text(json.dumps(good_evidence()), encoding="utf-8-sig")
        self.assertEqual(worker.load_low_spec_evidence(path)["kind"], "everward_unattended_low_spec_startup")
        path.write_text("not json", encoding="utf-8")
        self.assertIsNone(worker.load_low_spec_evidence(path))


class LowSpecStartupRunnerTests(unittest.TestCase):
    def test_non_windows_host_is_review_required(self) -> None:
        with tempfile.TemporaryDirectory() as temp, mock.patch.object(worker, "windows_host", return_value=False):
            repo = Path(temp)
            (repo / "tools").mkdir()
            (repo / "tools" / "run_unattended_low_spec_startup.ps1").write_text("", encoding="utf-8")
            check = worker.run_unreal_low_spec_startup(repo, repo, repo / "logs", COMMIT)
            self.assertEqual(check["status"], "REVIEW_REQUIRED")

    def test_missing_probe_script_is_review_required(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            check = worker.run_unreal_low_spec_startup(Path(temp), Path(temp), Path(temp) / "logs", COMMIT)
            self.assertEqual(check["status"], "REVIEW_REQUIRED")

    def test_running_editor_defers_without_launch(self) -> None:
        with tempfile.TemporaryDirectory() as temp, mock.patch.object(
            worker, "windows_host", return_value=True
        ), mock.patch.object(worker, "process_running", return_value=True), mock.patch.object(
            worker, "run_logged"
        ) as run_logged:
            repo = Path(temp)
            (repo / "tools").mkdir()
            (repo / "tools" / "run_unattended_low_spec_startup.ps1").write_text("", encoding="utf-8")
            check = worker.run_unreal_low_spec_startup(repo, repo, repo / "logs", COMMIT)
            self.assertEqual(check["status"], "REVIEW_REQUIRED")
            run_logged.assert_not_called()

    def test_stale_evidence_is_removed_before_probe_runs(self) -> None:
        with tempfile.TemporaryDirectory() as temp, mock.patch.object(
            worker, "windows_host", return_value=True
        ), mock.patch.object(worker, "process_running", return_value=False), mock.patch.object(
            worker, "run_logged", return_value=(0, 70.0)
        ):
            repo = Path(temp)
            (repo / "tools").mkdir()
            (repo / "tools" / "run_unattended_low_spec_startup.ps1").write_text("", encoding="utf-8")
            logs = repo / "logs"
            logs.mkdir()
            (logs / "low-spec-startup.json").write_text(json.dumps(good_evidence()), encoding="utf-8")
            check = worker.run_unreal_low_spec_startup(repo, repo, logs, COMMIT)
            self.assertEqual(check["status"], "REVIEW_REQUIRED")


class LowSpecStartupWorkerFlowTests(unittest.TestCase):
    def run_flow(self, low_spec: dict[str, Any], smoke_status: str = "PASS") -> tuple[dict[str, Any], Path, mock.MagicMock]:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        state = Path(temp.name) / "state"
        state.mkdir()
        repo = Path(temp.name) / "repo"
        passing = {"status": "PASS"}
        probe = mock.MagicMock(return_value=copy.deepcopy(low_spec))
        with mock.patch.multiple(
            worker,
            validate_dedicated_checkout=mock.MagicMock(return_value=None),
            manual_playtest_active=mock.MagicMock(return_value=False),
            latest_successful_main_sha=mock.MagicMock(return_value=COMMIT),
            sync_to_target=mock.MagicMock(return_value=(COMMIT, None)),
            run_full_preflight=mock.MagicMock(return_value=passing),
            resolve_unreal_root=mock.MagicMock(return_value=Path(temp.name) / "ue"),
            run_unreal_build=mock.MagicMock(return_value=passing),
            run_unreal_headless_smoke=mock.MagicMock(return_value={"status": smoke_status}),
            run_unreal_low_spec_startup=probe,
        ):
            report = worker.run_worker(repo, state)
        return report, state, probe

    def test_low_spec_pass_caches_exact_commit(self) -> None:
        report, state, probe = self.run_flow({"status": "PASS"})
        self.assertEqual(report["result"], "PASS")
        probe.assert_called_once()
        self.assertEqual(probe.call_args.args[3], COMMIT)
        self.assertEqual((state / worker.LAST_PASS_MARKER).read_text(encoding="ascii").strip(), COMMIT)
        self.assertIn("frame-time", " ".join(report["notes"]))

    def test_low_spec_review_required_does_not_cache_or_pass(self) -> None:
        report, state, _probe = self.run_flow({"status": "REVIEW_REQUIRED", "reason": "telemetry unavailable"})
        self.assertEqual(report["result"], "REVIEW_REQUIRED")
        self.assertFalse((state / worker.LAST_PASS_MARKER).exists())

    def test_low_spec_failure_does_not_cache_or_pass(self) -> None:
        report, state, _probe = self.run_flow({"status": "FAIL"})
        self.assertEqual(report["result"], "FAIL")
        self.assertFalse((state / worker.LAST_PASS_MARKER).exists())

    def test_low_spec_probe_is_not_attempted_after_failed_smoke(self) -> None:
        report, state, probe = self.run_flow({"status": "PASS"}, smoke_status="FAIL")
        self.assertEqual(report["result"], "FAIL")
        probe.assert_not_called()
        self.assertNotIn("unreal_low_spec_startup", report["checks"])
        self.assertFalse((state / worker.LAST_PASS_MARKER).exists())

    def test_human_only_debt_is_retained_after_low_spec_pass(self) -> None:
        report, _state, _probe = self.run_flow({"status": "PASS"})
        debt = " ".join(report["human_only_debt"])
        self.assertIn("visual/art-direction", debt)
        self.assertIn("control and movement feel", debt)
        self.assertIn("frame-time", debt)


class LowSpecStartupContractTests(unittest.TestCase):
    def test_probe_reuses_manual_low_spec_launch_contract_exactly(self) -> None:
        probe = PROBE_PATH.read_text(encoding="utf-8")
        harness = HARNESS_PATH.read_text(encoding="utf-8")
        self.assertEqual(low_spec_commands(probe), low_spec_commands(harness))
        for token in ('"-WINDOWED"', '"-ResX=1280"', '"-ResY=720"', '"-log"', "-ExecCmds="):
            self.assertIn(token, probe)
        self.assertIn("$SampleWindowSeconds = 60", harness)
        self.assertIn("$SampleWindowSeconds = 60", probe)
        self.assertIn("Start-Sleep -Milliseconds 500", harness)
        self.assertIn("$SampleIntervalMilliseconds = 500", probe)
        for token in ("WorkingSet64", "editor_peak_working_set_mib", "not total system RAM or shared-GPU memory"):
            self.assertIn(token, harness)
            self.assertIn(token, probe)

    def test_probe_is_bounded_and_cleans_up_its_process_tree(self) -> None:
        probe = PROBE_PATH.read_text(encoding="utf-8")
        self.assertIn("$StartupTimeoutSeconds", probe)
        self.assertIn("if ($Elapsed -ge $StartupTimeoutSeconds) { break }", probe)
        self.assertIn("taskkill.exe /PID $Process.Id /T /F", probe)
        self.assertIn('"-Unattended"', probe)
        self.assertIn("-abslog=", probe)
        self.assertIn("finally {", probe)

    def test_probe_does_not_change_production_settings_or_repository_files(self) -> None:
        probe = PROBE_PATH.read_text(encoding="utf-8")
        for forbidden in ("DefaultEngine.ini", "DefaultScalability", "GameUserSettings", "Set-ItemProperty", "git -C $RepoRoot checkout", "git -C $RepoRoot reset"):
            self.assertNotIn(forbidden, probe)
        self.assertNotIn("playtests\\phase2\\observations", probe)
        self.assertEqual(probe.count("Set-Content"), 1)
        self.assertIn("Set-Content -Path $EvidencePath", probe)

    def test_probe_evidence_contains_no_path_fields(self) -> None:
        probe = PROBE_PATH.read_text(encoding="utf-8")
        evidence_block = probe.split("$Evidence = [ordered]@{", 1)[1].split("$ExitCode = 3", 1)[0]
        for forbidden in ("$RepoRoot", "$ProjectPath", "$UnrealRoot", "$UnrealLogPath", "$EditorExe", "$EvidencePath"):
            self.assertNotIn(forbidden, evidence_block)

    def test_manual_launcher_remains_opt_in(self) -> None:
        harness = HARNESS_PATH.read_text(encoding="utf-8")
        self.assertIn('if ($MeasureMemory -and -not $LowSpec) { throw', harness)

    def test_publisher_allow_lists_low_spec_status_only(self) -> None:
        text = PUBLISH_PATH.read_text(encoding="utf-8")
        self.assertIn('"unreal_low_spec_startup"', text)
        self.assertNotIn("editor_peak_working_set_mib", text)


if __name__ == "__main__":
    unittest.main()
