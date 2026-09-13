from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"
REGISTER_PATH = ROOT / "tools" / "register_unattended_product_reality.ps1"
HARNESS_PATH = ROOT / "tools" / "run_phase2_first_playtest.ps1"

spec = importlib.util.spec_from_file_location("everward_unattended_worker", WORKER_PATH)
assert spec is not None and spec.loader is not None
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)


class UnattendedProductRealityWorkerTests(unittest.TestCase):
    def test_select_successful_main_sha_filters_non_push_and_failed_runs(self) -> None:
        wanted = "a" * 40
        payload = {
            "workflow_runs": [
                {"head_branch": "main", "conclusion": "success", "event": "workflow_dispatch", "head_sha": "b" * 40},
                {"head_branch": "main", "conclusion": "failure", "event": "push", "head_sha": "c" * 40},
                {"head_branch": "feature", "conclusion": "success", "event": "push", "head_sha": "d" * 40},
                {"head_branch": "main", "conclusion": "success", "event": "push", "head_sha": wanted},
            ]
        }
        self.assertEqual(worker.select_successful_main_sha(payload), wanted)

    def test_expected_origin_accepts_https_and_ssh(self) -> None:
        self.assertTrue(worker.origin_is_expected("https://github.com/jweter/Project-Everward.git"))
        self.assertTrue(worker.origin_is_expected("git@github.com:jweter/Project-Everward.git"))
        self.assertFalse(worker.origin_is_expected("https://github.com/someone/other.git"))

    def test_missing_dedicated_checkout_sentinel_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            (repo / ".git").mkdir()
            with self.assertRaisesRegex(RuntimeError, "sentinel is missing"):
                worker.validate_dedicated_checkout(repo)

    def test_explicit_unreal_root_is_discovered_from_required_files(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            build = root / "Engine" / "Build" / "BatchFiles" / "Build.bat"
            editor = root / "Engine" / "Binaries" / "Win64" / "UnrealEditor.exe"
            build.parent.mkdir(parents=True)
            editor.parent.mkdir(parents=True)
            build.write_text("@echo off\n", encoding="utf-8")
            editor.write_bytes(b"")
            self.assertEqual(worker.resolve_unreal_root(str(root)), root.resolve())

    def test_sanitize_text_removes_private_machine_paths(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp) / "Project-Everward"
            ue = Path(temp) / "UE_5.8"
            raw = f"error in {repo} while loading {ue} for {Path.home()}"
            sanitized = worker.sanitize_text(raw, repo_root=repo, unreal_root=ue)
            self.assertNotIn(str(repo), sanitized)
            self.assertNotIn(str(ue), sanitized)
            self.assertIn("<REPO_ROOT>", sanitized)
            self.assertIn("<UE_5_8_ROOT>", sanitized)

    def test_failure_dominates_review_required_and_pass(self) -> None:
        checks = {
            "one": {"status": "PASS"},
            "two": {"status": "REVIEW_REQUIRED"},
            "three": {"status": "FAIL"},
        }
        self.assertEqual(worker.aggregate_status(checks), "FAIL")
        self.assertEqual(worker.aggregate_status({"a": {"status": "REVIEW_REQUIRED"}}), "REVIEW_REQUIRED")
        self.assertEqual(worker.aggregate_status({"a": {"status": "PASS"}}), "PASS")

    def test_registration_script_is_idle_current_user_and_noninteractive(self) -> None:
        text = REGISTER_PATH.read_text(encoding="utf-8")
        self.assertIn('"Everward Unattended Product Reality"', text)
        self.assertIn("/SC ONIDLE", text)
        self.assertIn("/I 10", text)
        self.assertIn("/RL LIMITED", text)
        self.assertIn("/F", text)
        self.assertIn("/Delete", text)
        self.assertIn("EVERWARD_DISABLE_UNATTENDED_WORKER", text)
        self.assertNotIn("pause", text.lower())

    def test_worker_uses_foundation_green_commit_full_preflight_and_ubt_without_editor_launch(self) -> None:
        text = WORKER_PATH.read_text(encoding="utf-8")
        self.assertIn('FOUNDATION_WORKFLOW = "foundation.yml"', text)
        self.assertIn('"quality_preflight.py"', text)
        self.assertIn('"--full"', text)
        self.assertIn('"EverwardEditor"', text)
        self.assertIn('"Win64"', text)
        self.assertIn('"Development"', text)
        self.assertIn('"-NoHotReloadFromIDE"', text)
        self.assertNotIn("Start-Process", text)
        self.assertNotIn('"-log"', text)

    def test_manual_harness_silently_attempts_worker_registration(self) -> None:
        text = HARNESS_PATH.read_text(encoding="utf-8")
        self.assertIn("register_unattended_product_reality.ps1", text)
        self.assertIn("Manual playtest will continue", text)


if __name__ == "__main__":
    unittest.main()
