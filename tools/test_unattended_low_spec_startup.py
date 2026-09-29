from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"

spec = importlib.util.spec_from_file_location("everward_unattended_worker_low_spec", WORKER_PATH)
assert spec is not None and spec.loader is not None
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)

COMMIT = "a" * 40
OTHER_COMMIT = "b" * 40


def _passing_evidence(commit: str = COMMIT) -> dict:
    return {
        "schema_version": 1,
        "profile": "low_spec_development",
        "git_commit": commit,
        "editor_process_detected": True,
        "exited_during_sample_window": False,
        "exit_code": None,
        "peak_working_set_mib": 812.5,
        "sample_window_seconds": 60,
        "product_reality_claimed": False,
        "interpretation": "process working set only; not visual, control-feel, or gameplay Product Reality",
    }


class EvaluateLowSpecStartupEvidenceTests(unittest.TestCase):
    def test_non_dict_evidence_is_review_required(self) -> None:
        for bad in (None, "not json", 42, ["a"]):
            with self.subTest(bad=bad):
                result = worker.evaluate_low_spec_startup_evidence(bad, COMMIT)
                self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_missing_or_mismatched_commit_is_review_required(self) -> None:
        for evidence in (
            {**_passing_evidence(), "git_commit": ""},
            {**_passing_evidence(), "git_commit": OTHER_COMMIT},
            {k: v for k, v in _passing_evidence().items() if k != "git_commit"},
        ):
            with self.subTest(evidence=evidence):
                result = worker.evaluate_low_spec_startup_evidence(evidence, COMMIT)
                self.assertEqual(result["status"], "REVIEW_REQUIRED")
                self.assertIn("commit", result["reason"])

    def test_editor_not_detected_is_review_required_not_pass(self) -> None:
        evidence = {**_passing_evidence(), "editor_process_detected": False}
        result = worker.evaluate_low_spec_startup_evidence(evidence, COMMIT)
        self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_editor_exit_during_sample_window_is_fail(self) -> None:
        evidence = {
            **_passing_evidence(),
            "exited_during_sample_window": True,
            "exit_code": 3,
        }
        result = worker.evaluate_low_spec_startup_evidence(evidence, COMMIT)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["exit_code"], 3)

    def test_unavailable_memory_telemetry_is_review_required(self) -> None:
        mutations = (
            {"peak_working_set_mib": None},
            {"sample_window_seconds": None},
            {"peak_working_set_mib": "unknown"},
            {"peak_working_set_mib": True},
            {"sample_window_seconds": False},
        )
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                evidence = {**_passing_evidence(), **mutation}
                result = worker.evaluate_low_spec_startup_evidence(evidence, COMMIT)
                self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_complete_bounded_evidence_passes_without_product_reality_claim(self) -> None:
        result = worker.evaluate_low_spec_startup_evidence(_passing_evidence(), COMMIT)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["peak_working_set_mib"], 812.5)
        self.assertEqual(result["sample_window_seconds"], 60)
        self.assertIn("not visual", result["interpretation"])
        self.assertIn("gameplay", result["interpretation"])

    def test_commit_match_is_case_insensitive(self) -> None:
        evidence = {**_passing_evidence(), "git_commit": COMMIT.upper()}
        result = worker.evaluate_low_spec_startup_evidence(evidence, COMMIT)
        self.assertEqual(result["status"], "PASS")

    def test_evidence_claiming_product_reality_is_rejected(self) -> None:
        evidence = {**_passing_evidence(), "product_reality_claimed": True}
        result = worker.evaluate_low_spec_startup_evidence(evidence, COMMIT)
        self.assertNotIn("product_reality_claimed", result)
        self.assertIn("not visual, control-feel, or gameplay Product Reality", result["interpretation"])

    def test_only_allow_listed_facts_are_copied_so_private_paths_cannot_leak(self) -> None:
        evidence = {
            **_passing_evidence(),
            "source": r"C:\Users\someone\Documents\Everward Playtest\Project-Everward",
            "unreal_log": "crash in C:\\Users\\someone\\AppData\\Local\\Everward\\Saved\\Logs\\Everward.log",
            "launch_contract": "private-internal-detail",
        }
        result = worker.evaluate_low_spec_startup_evidence(evidence, COMMIT)
        serialized = json.dumps(result)
        for private in ("someone", "Everward Playtest", "crash in", "launch_contract", "unreal_log", "source"):
            self.assertNotIn(private, serialized)


class RunLowSpecStartupTests(unittest.TestCase):
    def _make_repo(self, *, with_harness: bool = True) -> Path:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        repo_root = Path(temp.name)
        if with_harness:
            harness = repo_root / "tools" / "run_phase2_first_playtest.ps1"
            harness.parent.mkdir(parents=True, exist_ok=True)
            harness.write_text("# stub\n", encoding="utf-8")
        return repo_root

    def test_missing_harness_script_fails_closed(self) -> None:
        repo_root = self._make_repo(with_harness=False)
        with tempfile.TemporaryDirectory() as work:
            result = worker.run_low_spec_startup(
                repo_root,
                Path(work) / "UE_5.8",
                COMMIT,
                Path(work) / "low-spec-startup.log",
                Path(work) / "low-spec-startup.json",
            )
        self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_nonzero_exit_is_fail_with_sanitized_failure_tail(self) -> None:
        repo_root = self._make_repo()
        unreal_root = repo_root.parent / "UE_5.8"

        def fake_run_logged(args, *, cwd, log_path, timeout_seconds):
            log_path.parent.mkdir(parents=True, exist_ok=True)
            log_path.write_text(f"launch failed under {repo_root} using {unreal_root}\n", encoding="utf-8")
            return 1, 3.5

        with tempfile.TemporaryDirectory() as work, mock.patch.object(
            worker, "run_logged", side_effect=fake_run_logged
        ):
            result = worker.run_low_spec_startup(
                repo_root,
                unreal_root,
                COMMIT,
                Path(work) / "low-spec-startup.log",
                Path(work) / "low-spec-startup.json",
            )
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["exit_code"], 1)
        self.assertNotIn(str(repo_root), result["failure_tail"])
        self.assertNotIn(str(unreal_root), result["failure_tail"])
        self.assertIn("<REPO_ROOT>", result["failure_tail"])
        self.assertIn("<UE_5_8_ROOT>", result["failure_tail"])

    def test_missing_sidecar_after_success_exit_is_review_required_not_pass(self) -> None:
        repo_root = self._make_repo()

        def fake_run_logged(args, *, cwd, log_path, timeout_seconds):
            return 0, 1.0

        with tempfile.TemporaryDirectory() as work, mock.patch.object(
            worker, "run_logged", side_effect=fake_run_logged
        ):
            result = worker.run_low_spec_startup(
                repo_root,
                repo_root.parent / "UE_5.8",
                COMMIT,
                Path(work) / "low-spec-startup.log",
                Path(work) / "low-spec-startup.json",
            )
        self.assertEqual(result["status"], "REVIEW_REQUIRED")
        self.assertNotEqual(result["status"], "PASS")

    def test_unparsable_sidecar_is_review_required(self) -> None:
        repo_root = self._make_repo()

        def fake_run_logged(args, *, cwd, log_path, timeout_seconds):
            evidence_path = Path(args[args.index("-LowSpecEvidencePath") + 1])
            evidence_path.write_text("not json", encoding="utf-8")
            return 0, 1.0

        with tempfile.TemporaryDirectory() as work, mock.patch.object(
            worker, "run_logged", side_effect=fake_run_logged
        ):
            result = worker.run_low_spec_startup(
                repo_root,
                repo_root.parent / "UE_5.8",
                COMMIT,
                Path(work) / "low-spec-startup.log",
                Path(work) / "low-spec-startup.json",
            )
        self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_stale_sidecar_from_a_prior_run_is_not_silently_reused(self) -> None:
        repo_root = self._make_repo()

        def fake_run_logged(args, *, cwd, log_path, timeout_seconds):
            # Simulates a run that exits 0 but fails to (re)write fresh evidence.
            return 0, 1.0

        with tempfile.TemporaryDirectory() as work:
            evidence_path = Path(work) / "low-spec-startup.json"
            evidence_path.write_text(json.dumps(_passing_evidence()), encoding="utf-8")
            with mock.patch.object(worker, "run_logged", side_effect=fake_run_logged):
                result = worker.run_low_spec_startup(
                    repo_root,
                    repo_root.parent / "UE_5.8",
                    COMMIT,
                    Path(work) / "low-spec-startup.log",
                    evidence_path,
                )
            # Checked while the temp directory is still alive, so this proves the
            # stale file was actually unlinked rather than merely being gone due
            # to directory teardown.
            self.assertFalse(evidence_path.exists())
        self.assertEqual(result["status"], "REVIEW_REQUIRED")

    def test_valid_evidence_passes_with_exact_commit_and_memory_facts(self) -> None:
        repo_root = self._make_repo()
        captured_args: list[str] = []

        def fake_run_logged(args, *, cwd, log_path, timeout_seconds):
            captured_args.extend(args)
            evidence_path = Path(args[args.index("-LowSpecEvidencePath") + 1])
            evidence_path.write_text(json.dumps(_passing_evidence()), encoding="utf-8")
            return 0, 42.0

        with tempfile.TemporaryDirectory() as work, mock.patch.object(
            worker, "run_logged", side_effect=fake_run_logged
        ):
            result = worker.run_low_spec_startup(
                repo_root,
                repo_root.parent / "UE_5.8",
                COMMIT,
                Path(work) / "low-spec-startup.log",
                Path(work) / "low-spec-startup.json",
            )
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["peak_working_set_mib"], 812.5)
        self.assertIn("gameplay Product Reality", result["interpretation"])
        for expected in ("-LowSpec", "-MeasureMemory", "-UnattendedBoundedExit", "-SkipBuild"):
            self.assertIn(expected, captured_args)


if __name__ == "__main__":
    unittest.main()
