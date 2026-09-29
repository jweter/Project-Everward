from __future__ import annotations

import importlib.util
import json
import re
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
STARTUP_SCRIPT = ROOT / "tools" / "run_unreal_low_spec_startup_check.ps1"
HARNESS_SCRIPT = ROOT / "tools" / "run_phase2_first_playtest.ps1"
HEADLESS_SMOKE_SCRIPT = ROOT / "tools" / "run_unreal_headless_smoke.ps1"
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"
PUBLISH_PATH = ROOT / "tools" / "publish_unattended_product_reality.ps1"

spec = importlib.util.spec_from_file_location("everward_unattended_worker", WORKER_PATH)
assert spec is not None and spec.loader is not None
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)


def _lowspec_exec_cmds(source: str) -> str:
    match = re.search(r"\$LowSpecCommands = (@\([^\n]+\))", source)
    assert match is not None, "LowSpec exec-command list was not found"
    return match.group(1)


class LowSpecStartupCheckScriptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.source = STARTUP_SCRIPT.read_text(encoding="utf-8")
        cls.harness_source = HARNESS_SCRIPT.read_text(encoding="utf-8")

    def test_reuses_exact_low_spec_exec_cmds_contract_from_manual_harness(self) -> None:
        self.assertEqual(
            _lowspec_exec_cmds(self.source), _lowspec_exec_cmds(self.harness_source)
        )

    def test_enforces_same_bounded_cold_start_floor_as_headless_smoke(self) -> None:
        headless = HEADLESS_SMOKE_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("[Math]::Max($TimeoutSeconds, 900)", headless)
        self.assertIn(
            "$EffectiveStartupTimeoutSeconds = [Math]::Max($StartupTimeoutSeconds, 900)",
            self.source,
        )

    def test_binds_evidence_and_log_to_exact_commit(self) -> None:
        self.assertIn("git -C $RepoRoot rev-parse HEAD", self.source)
        self.assertIn('"low-spec-startup-$GitCommit.log"', self.source)
        self.assertIn('"$GitCommit.json"', self.source)

    def test_always_terminates_the_editor_process_in_a_finally_block(self) -> None:
        finally_index = self.source.index("finally {")
        taskkill_index = self.source.index('taskkill.exe /PID $Process.Id /T /F')
        self.assertGreater(taskkill_index, finally_index)
        self.assertIn("-not $Process.HasExited", self.source[finally_index:])

    def test_fails_closed_on_startup_exit_timeout_crash_and_missing_sample(self) -> None:
        for expected in (
            'Write-StartupEvidence -Result "fail" -Reason "UnrealEditor exited during startup',
            "Low-spec Unreal Editor startup failed: process exited during startup",
            "Low-spec Unreal Editor startup timed out after",
            "Low-spec Unreal Editor crashed during the memory sample window",
            "Low-spec Unreal Editor startup produced no memory evidence",
        ):
            self.assertIn(expected, self.source)

    def test_never_claims_visual_or_gameplay_product_reality(self) -> None:
        for forbidden in ("PRODUCT_REALITY_VERIFIED", "Product Reality PASS", "gameplay feel"):
            self.assertNotIn(forbidden, self.source)
        self.assertIn(
            "a visual/control-feel/gameplay Product Reality signal", self.source
        )

    def test_evidence_schema_records_bounded_facts_only(self) -> None:
        for field in (
            "commit = $GitCommit",
            "profile = \"low_spec_v1\"",
            "startup_result = $Result",
            "peak_working_set_mib = $PeakWorkingSetMiB",
            "sample_window_seconds = $WindowSeconds",
        ):
            self.assertIn(field, self.source)


class LowSpecStartupPublisherTests(unittest.TestCase):
    def test_publisher_surfaces_low_spec_startup_status_without_raw_evidence(self) -> None:
        source = PUBLISH_PATH.read_text(encoding="utf-8")
        self.assertIn('"low_spec_startup"', source)
        self.assertNotIn("peak_working_set_mib", source)
        self.assertNotIn("$Check.failure_tail)", source)


class LowSpecStartupWorkerIntegrationTests(unittest.TestCase):
    def _stub_repo(self, temp: Path) -> Path:
        repo = temp / "repo"
        (repo / "tools").mkdir(parents=True)
        (repo / "tools" / "run_unreal_low_spec_startup_check.ps1").write_text(
            "# stub", encoding="utf-8"
        )
        return repo

    def test_review_required_when_evidence_file_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            repo = self._stub_repo(temp_path)
            state_dir = temp_path / "state"
            with mock.patch.object(
                worker, "default_state_dir", return_value=state_dir
            ), mock.patch.object(worker, "run_logged", return_value=(0, 1.0)):
                result = worker.run_unreal_low_spec_startup(
                    repo, temp_path / "UE_5.8", temp_path / "log.txt", "a" * 40
                )
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertIn("evidence", result["reason"].lower())

    def test_review_required_when_evidence_commit_does_not_match_tested_commit(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            repo = self._stub_repo(temp_path)
            state_dir = temp_path / "state"
            evidence_dir = state_dir / "low-spec-startup"
            evidence_dir.mkdir(parents=True)
            tested_commit = "a" * 40
            (evidence_dir / f"{tested_commit}.json").write_text(
                json.dumps(
                    {
                        "commit": "b" * 40,
                        "startup_result": "pass",
                        "peak_working_set_mib": 512.0,
                        "sample_window_seconds": 60,
                    }
                ),
                encoding="utf-8",
            )
            with mock.patch.object(
                worker, "default_state_dir", return_value=state_dir
            ), mock.patch.object(worker, "run_logged", return_value=(0, 1.0)):
                result = worker.run_unreal_low_spec_startup(
                    repo, temp_path / "UE_5.8", temp_path / "log.txt", tested_commit
                )
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertIn("did not match", result["reason"])

    def test_review_required_when_evidence_reports_non_pass_despite_zero_exit(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            repo = self._stub_repo(temp_path)
            state_dir = temp_path / "state"
            evidence_dir = state_dir / "low-spec-startup"
            evidence_dir.mkdir(parents=True)
            tested_commit = "a" * 40
            (evidence_dir / f"{tested_commit}.json").write_text(
                json.dumps(
                    {
                        "commit": tested_commit,
                        "startup_result": "fail",
                        "reason": "UnrealEditor exited during startup with exit code -1.",
                        "peak_working_set_mib": 0,
                        "sample_window_seconds": 0,
                    }
                ),
                encoding="utf-8",
            )
            with mock.patch.object(
                worker, "default_state_dir", return_value=state_dir
            ), mock.patch.object(worker, "run_logged", return_value=(0, 1.0)):
                result = worker.run_unreal_low_spec_startup(
                    repo, temp_path / "UE_5.8", temp_path / "log.txt", tested_commit
                )
            self.assertEqual(result["status"], "REVIEW_REQUIRED")
            self.assertEqual(result["evidence"]["startup_result"], "fail")

    def test_fail_status_is_preserved_when_process_exit_code_is_nonzero(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            repo = self._stub_repo(temp_path)
            state_dir = temp_path / "state"
            with mock.patch.object(
                worker, "default_state_dir", return_value=state_dir
            ), mock.patch.object(worker, "run_logged", return_value=(1, 1.0)):
                result = worker.run_unreal_low_spec_startup(
                    repo, temp_path / "UE_5.8", temp_path / "log.txt", "a" * 40
                )
            self.assertEqual(result["status"], "FAIL")

    def test_passes_only_when_evidence_commit_matches_and_reports_pass(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            repo = self._stub_repo(temp_path)
            state_dir = temp_path / "state"
            evidence_dir = state_dir / "low-spec-startup"
            evidence_dir.mkdir(parents=True)
            tested_commit = "a" * 40
            (evidence_dir / f"{tested_commit}.json").write_text(
                json.dumps(
                    {
                        "commit": tested_commit,
                        "profile": "low_spec_v1",
                        "startup_result": "pass",
                        "peak_working_set_mib": 812.3,
                        "sample_window_seconds": 60,
                        "interpretation": "process working set only",
                    }
                ),
                encoding="utf-8",
            )
            with mock.patch.object(
                worker, "default_state_dir", return_value=state_dir
            ), mock.patch.object(worker, "run_logged", return_value=(0, 1.0)):
                result = worker.run_unreal_low_spec_startup(
                    repo, temp_path / "UE_5.8", temp_path / "log.txt", tested_commit
                )
            self.assertEqual(result["status"], "PASS")
            self.assertEqual(result["evidence"]["peak_working_set_mib"], 812.3)

    def test_missing_script_fails_closed_to_review_required(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            temp_path = Path(temp)
            repo = temp_path / "repo"
            (repo / "tools").mkdir(parents=True)
            result = worker.run_unreal_low_spec_startup(
                repo, temp_path / "UE_5.8", temp_path / "log.txt", "a" * 40
            )
            self.assertEqual(result["status"], "REVIEW_REQUIRED")


class LowSpecStartupWorkerOrderingTests(unittest.TestCase):
    def test_low_spec_startup_runs_only_after_headless_smoke_passes(self) -> None:
        source = WORKER_PATH.read_text(encoding="utf-8")
        run_body = source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]
        self.assertIn("run_unreal_low_spec_startup(", run_body)
        smoke_index = run_body.index("run_unreal_headless_smoke(")
        pass_branch_index = run_body.index('headless_smoke.get("status") == "PASS"')
        startup_index = run_body.index("run_unreal_low_spec_startup(", smoke_index)
        self.assertLess(smoke_index, pass_branch_index)
        self.assertLess(pass_branch_index, startup_index)

    def test_headless_smoke_cache_marker_still_precedes_low_spec_startup_call(self) -> None:
        source = WORKER_PATH.read_text(encoding="utf-8")
        run_body = source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]
        marker_index = run_body.index("LAST_PASS_MARKER).write_text")
        startup_index = run_body.index("run_unreal_low_spec_startup(")
        self.assertLess(marker_index, startup_index)

    def test_non_pass_low_spec_startup_can_still_dominate_final_result(self) -> None:
        source = WORKER_PATH.read_text(encoding="utf-8")
        run_body = source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]
        startup_index = run_body.index('report["checks"]["low_spec_startup"] = low_spec_startup')
        aggregate_index = run_body.rindex('report["result"] = aggregate_status(report["checks"])')
        self.assertLess(startup_index, aggregate_index)


if __name__ == "__main__":
    unittest.main()
