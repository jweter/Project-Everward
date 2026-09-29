from __future__ import annotations

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
HARNESS_PATH = ROOT / "tools" / "run_phase2_first_playtest.ps1"
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"
PUBLISH_PATH = ROOT / "tools" / "publish_unattended_product_reality.ps1"


class LowSpecStartupHarnessContractTests(unittest.TestCase):
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
        self.assertIn("[string]$LowSpecEvidencePath", self.source)
        self.assertIn("-UnattendedBoundedExit requires -LowSpecEvidencePath.", self.source)

    def test_bounded_exit_records_exact_commit_and_memory_facts(self) -> None:
        self.assertIn("git_commit = $GitCommit", self.source)
        self.assertIn("editor_process_detected = $EditorProcessDetected", self.source)
        self.assertIn("exited_during_sample_window = $ExitedDuringSampleWindow", self.source)
        self.assertIn("peak_working_set_mib = $PeakWorkingSetMiB", self.source)
        self.assertIn("sample_window_seconds = $SampleWindowSeconds", self.source)
        self.assertIn("product_reality_claimed = $false", self.source)

    def test_bounded_exit_terminates_the_editor_process_tree(self) -> None:
        self.assertIn("taskkill.exe /PID $EditorProcess.Id /T /F", self.source)
        self.assertIn("Stop-Process -Id $EditorProcess.Id -Force", self.source)
        # Cleanup must run even if writing the evidence file throws.
        bounded_block = self.source.split("if ($UnattendedBoundedExit) {", 1)[1]
        self.assertLess(bounded_block.index("try {"), bounded_block.index("finally {"))

    def test_bounded_exit_logic_is_nested_under_low_spec_memory_and_its_own_switch(self) -> None:
        measure_index = self.source.index("if ($LowSpec -and $MeasureMemory) {")
        bounded_index = self.source.index("if ($UnattendedBoundedExit) {")
        sidecar_index = self.source.index("$LowSpecEvidencePath -Encoding UTF8")
        kill_index = self.source.index("taskkill.exe /PID $EditorProcess.Id /T /F")
        self.assertLess(measure_index, bounded_index)
        self.assertLess(bounded_index, sidecar_index)
        self.assertLess(sidecar_index, kill_index)

    def test_default_manual_launch_message_is_unaffected_by_bounded_exit(self) -> None:
        self.assertIn("if (-not $UnattendedBoundedExit) {", self.source)
        self.assertIn(
            "Unreal Editor launched. Follow docs/PHASE2_FIRST_RUN_PLAYTEST.md and update the observation file as you test.",
            self.source,
        )

    def test_evidence_block_never_writes_a_raw_path_variable_as_a_fact(self) -> None:
        bounded_block = self.source.split("$BoundedEvidence = [ordered]@{", 1)[1].split("}", 1)[0]
        for path_variable in ("$RepoRoot", "$ProjectPath", "$UnrealRoot", "$EditorExe", "$ObservationPath"):
            self.assertNotIn(path_variable, bounded_block)

    def test_bounded_exit_does_not_touch_production_settings_or_simulation(self) -> None:
        for forbidden in (
            "DefaultEngine.ini",
            "DefaultScalability",
            "GameUserSettings",
            "src/simulation",
            "src\\simulation",
        ):
            self.assertNotIn(forbidden, self.source)

    def test_manual_harness_default_switches_are_unchanged(self) -> None:
        # Backward compatibility: the pre-existing manual/human contract is untouched.
        self.assertIn("[switch]$LowSpec", self.source)
        self.assertIn("[switch]$MeasureMemory", self.source)
        self.assertIn("-MeasureMemory is currently restricted to -LowSpec evidence runs.", self.source)


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
        self.assertIn('report["checks"]["low_spec_startup"] = low_spec_startup', self.source)
        aggregate_index = self.run_body.index("aggregate_status(report[\"checks\"])", self.run_body.index("low_spec_startup"))
        self.assertGreater(aggregate_index, self.run_body.index("low_spec_startup"))

    def test_pass_notes_do_not_overclaim_visual_or_gameplay_reality(self) -> None:
        self.assertIn("bounded low-spec startup/memory evidence", self.run_body)
        self.assertIn("remain human-only", self.run_body)

    def test_evidence_is_bound_to_the_exact_synced_commit_not_the_discovered_target(self) -> None:
        low_spec_call = self.run_body[self.run_body.index("run_low_spec_startup(") :]
        low_spec_call = low_spec_call[: low_spec_call.index(")\n") + 2]
        self.assertIn("synced", low_spec_call)

    def test_evaluator_and_runner_are_defined(self) -> None:
        self.assertIn("def evaluate_low_spec_startup_evidence(", self.source)
        self.assertIn("def run_low_spec_startup(", self.source)
        self.assertIn('LOW_SPEC_STARTUP_SIDECAR = "low-spec-startup.json"', self.source)

    def test_sidecar_is_read_with_bom_tolerant_encoding(self) -> None:
        self.assertIn('encoding="utf-8-sig"', self.source)

    def test_stale_evidence_is_discarded_before_relaunching(self) -> None:
        runner_body = self.source.split("def run_low_spec_startup(", 1)[1].split("def acquire_lock", 1)[0]
        unlink_index = runner_body.index("evidence_path.unlink(missing_ok=True)")
        run_logged_index = runner_body.index("run_logged(")
        self.assertLess(unlink_index, run_logged_index)


class LowSpecStartupPublisherTests(unittest.TestCase):
    def test_publisher_lists_low_spec_check_with_fingerprint_only(self) -> None:
        source = PUBLISH_PATH.read_text(encoding="utf-8")
        self.assertIn('"low_spec_startup"', source)
        self.assertNotIn("$Check.peak_working_set_mib", source)


if __name__ == "__main__":
    unittest.main()
