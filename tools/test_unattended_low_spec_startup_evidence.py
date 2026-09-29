from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"
HARNESS_PATH = ROOT / "tools" / "run_phase2_first_playtest.ps1"

spec = importlib.util.spec_from_file_location("everward_unattended_worker", WORKER_PATH)
assert spec is not None and spec.loader is not None
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)


class LowSpecStartupLauncherContractTests(unittest.TestCase):
    """The unattended bounded-exit mode must only extend, never alter, the manual harness."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.source = HARNESS_PATH.read_text(encoding="utf-8")

    def test_bounded_exit_is_explicit_and_requires_low_spec_memory_profile(self) -> None:
        self.assertIn("[switch]$UnattendedBoundedExit", self.source)
        self.assertIn(
            'if ($UnattendedBoundedExit -and -not ($LowSpec -and $MeasureMemory)) '
            '{ throw "-UnattendedBoundedExit requires both -LowSpec and -MeasureMemory." }',
            self.source,
        )

    def test_bounded_exit_records_exact_commit_and_memory_sidecar(self) -> None:
        self.assertIn("unattended-low-spec-startup.json", self.source)
        self.assertIn("git_commit = $GitCommit", self.source)
        self.assertIn("editor_process_detected", self.source)
        self.assertIn("peak_working_set_mib = $PeakWorkingSetMiB", self.source)
        self.assertIn("sample_window_seconds = $SampleWindowSeconds", self.source)

    def test_bounded_exit_terminates_the_editor_process_tree(self) -> None:
        self.assertIn("taskkill.exe /PID $EditorProcess.Id /T /F", self.source)
        self.assertIn("Stop-Process -Id $EditorProcess.Id -Force", self.source)

    def test_bounded_exit_logic_is_nested_under_low_spec_memory_and_its_own_switch(self) -> None:
        measure_index = self.source.index("if ($LowSpec -and $MeasureMemory) {")
        bounded_index = self.source.index("if ($UnattendedBoundedExit) {", measure_index)
        sidecar_index = self.source.index("unattended-low-spec-startup.json", bounded_index)
        kill_index = self.source.index("taskkill.exe /PID $EditorProcess.Id /T /F", bounded_index)
        self.assertLess(measure_index, bounded_index)
        self.assertLess(bounded_index, sidecar_index)
        self.assertLess(sidecar_index, kill_index)

    def test_default_manual_launch_message_is_unaffected_by_bounded_exit(self) -> None:
        self.assertIn("if (-not $UnattendedBoundedExit) {", self.source)
        self.assertIn(
            "Unreal Editor launched. Follow docs/PHASE2_FIRST_RUN_PLAYTEST.md and update the observation file as you test.",
            self.source,
        )


class LowSpecStartupWorkerIntegrationTests(unittest.TestCase):
    """The evidence lane must only run after headless smoke passes and must never upgrade to PASS on missing evidence."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.source = WORKER_PATH.read_text(encoding="utf-8")
        cls.run_body = cls.source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]

    def test_low_spec_startup_runs_only_after_headless_smoke_passes(self) -> None:
        smoke_index = self.run_body.index("run_unreal_headless_smoke(")
        guard_index = self.run_body.index('headless_smoke.get("status") != "PASS"')
        low_spec_index = self.run_body.index("run_low_spec_startup(")
        marker_index = self.run_body.index("LAST_PASS_MARKER).write_text")
        self.assertLess(smoke_index, guard_index)
        self.assertLess(guard_index, low_spec_index)
        self.assertLess(low_spec_index, marker_index)

    def test_low_spec_startup_check_feeds_the_shared_aggregate_status(self) -> None:
        self.assertIn('report["checks"]["low_spec_startup"] = low_spec_startup', self.run_body)

    def test_pass_notes_do_not_overclaim_visual_or_gameplay_reality(self) -> None:
        self.assertIn("bounded low-spec startup/memory evidence", self.run_body)
        self.assertIn("remain human-only", self.run_body)

    def test_cached_pass_semantics_mention_low_spec_evidence(self) -> None:
        self.assertIn("plus bounded low-spec startup/memory evidence", self.run_body)


class LowSpecStartupCheckUnitTests(unittest.TestCase):
    def _make_repo(self, tmp: Path) -> Path:
        repo_root = tmp / "repo"
        (repo_root / "tools").mkdir(parents=True)
        (repo_root / "tools" / "run_phase2_first_playtest.ps1").write_text("# stub\n", encoding="utf-8")
        return repo_root

    @staticmethod
    def _run_logged_writing_sidecar(sidecar_path: Path, payload: dict, *, exit_code: int = 0, duration: float = 30.0):
        # Mimics the real bounded-exit script: the sidecar only exists because
        # the invoked process wrote it, not because a prior run left it behind.
        def _fake(*_args, **_kwargs):
            sidecar_path.parent.mkdir(parents=True, exist_ok=True)
            sidecar_path.write_text(json.dumps(payload), encoding="utf-8")
            return (exit_code, duration)

        return _fake

    def test_missing_harness_script_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo_root = Path(tmp) / "repo"
            repo_root.mkdir()
            result = worker.run_low_spec_startup(
                repo_root, repo_root / "UE_5.8", repo_root / "log.log"
            )
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertIn("run_phase2_first_playtest.ps1", result["reason"])

    def test_non_windows_host_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo_root = self._make_repo(Path(tmp))
            with mock.patch.object(worker.os, "name", "posix"):
                result = worker.run_low_spec_startup(
                    repo_root, repo_root / "UE_5.8", repo_root / "log.log"
                )
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertIn("Windows", result["reason"])

    def test_nonzero_exit_is_fail_with_sanitized_failure_tail(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo_root = self._make_repo(Path(tmp))
            unreal_root = Path(tmp) / "UE_5.8"
            log_path = repo_root / "logs" / "low-spec-startup.log"
            log_path.parent.mkdir(parents=True)
            log_path.write_text(
                f"launch failed under {repo_root} using {unreal_root}\n", encoding="utf-8"
            )
            with mock.patch.object(worker, "run_logged", return_value=(1, 4.5)):
                result = worker.run_low_spec_startup(repo_root, unreal_root, log_path)
            self.assertEqual(result["status"], "FAIL")
            self.assertNotIn(str(repo_root), result["failure_tail"])
            self.assertNotIn(str(unreal_root), result["failure_tail"])
            self.assertIn("<REPO_ROOT>", result["failure_tail"])
            self.assertIn("<UE_5_8_ROOT>", result["failure_tail"])

    def test_missing_sidecar_after_success_exit_is_review_required_not_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo_root = self._make_repo(Path(tmp))
            unreal_root = Path(tmp) / "UE_5.8"
            log_path = repo_root / "logs" / "low-spec-startup.log"
            log_path.parent.mkdir(parents=True)
            log_path.write_text("ok\n", encoding="utf-8")
            with mock.patch.object(worker, "run_logged", return_value=(0, 30.0)):
                result = worker.run_low_spec_startup(repo_root, unreal_root, log_path)
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertNotEqual(result["status"], "PASS")

    def test_stale_sidecar_from_a_prior_run_is_not_silently_reused(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo_root = self._make_repo(Path(tmp))
            unreal_root = Path(tmp) / "UE_5.8"
            log_path = repo_root / "logs" / "low-spec-startup.log"
            log_path.parent.mkdir(parents=True)
            log_path.write_text("ok\n", encoding="utf-8")
            sidecar_path = repo_root / worker.LOW_SPEC_STARTUP_SIDECAR
            sidecar_path.parent.mkdir(parents=True)
            sidecar_path.write_text(
                json.dumps(
                    {
                        "git_commit": "f" * 40,
                        "editor_process_detected": True,
                        "peak_working_set_mib": 512.0,
                        "sample_window_seconds": 60,
                    }
                ),
                encoding="utf-8",
            )
            with mock.patch.object(worker, "run_logged", return_value=(0, 30.0)):
                result = worker.run_low_spec_startup(repo_root, unreal_root, log_path)
            self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_editor_not_detected_is_review_required_not_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo_root = self._make_repo(Path(tmp))
            unreal_root = Path(tmp) / "UE_5.8"
            log_path = repo_root / "logs" / "low-spec-startup.log"
            log_path.parent.mkdir(parents=True)
            log_path.write_text("ok\n", encoding="utf-8")
            sidecar_path = repo_root / worker.LOW_SPEC_STARTUP_SIDECAR
            payload = {
                "git_commit": "a" * 40,
                "editor_process_detected": False,
                "peak_working_set_mib": 0,
                "sample_window_seconds": 60,
            }
            with mock.patch.object(
                worker, "run_logged", side_effect=self._run_logged_writing_sidecar(sidecar_path, payload)
            ):
                result = worker.run_low_spec_startup(repo_root, unreal_root, log_path)
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertFalse(sidecar_path.exists())

    def test_valid_evidence_passes_with_exact_commit_and_memory_facts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo_root = self._make_repo(Path(tmp))
            unreal_root = Path(tmp) / "UE_5.8"
            log_path = repo_root / "logs" / "low-spec-startup.log"
            log_path.parent.mkdir(parents=True)
            log_path.write_text("ok\n", encoding="utf-8")
            sidecar_path = repo_root / worker.LOW_SPEC_STARTUP_SIDECAR
            commit = "b" * 40
            payload = {
                "git_commit": commit,
                "editor_process_detected": True,
                "peak_working_set_mib": 1024.5,
                "sample_window_seconds": 60,
            }
            with mock.patch.object(
                worker, "run_logged", side_effect=self._run_logged_writing_sidecar(sidecar_path, payload, duration=45.0)
            ):
                result = worker.run_low_spec_startup(repo_root, unreal_root, log_path)
            self.assertEqual(result["status"], "PASS")
            self.assertEqual(result["tested_commit"], commit)
            self.assertEqual(result["peak_working_set_mib"], 1024.5)
            self.assertEqual(result["sample_window_seconds"], 60)
            self.assertIn("not visual, control-feel, or gameplay Product Reality", result["interpretation"])
            self.assertFalse(sidecar_path.exists())


if __name__ == "__main__":
    unittest.main()
