from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKER = ROOT / "tools/everward_unattended_worker.py"
REGISTER = ROOT / "tools/register_unattended_product_reality.ps1"
HARNESS = ROOT / "tools/run_phase2_first_playtest.ps1"
SETUP = ROOT / "tools/Setup_Everward_Unattended_Testing.bat"


class UnattendedSafetyRegressionTests(unittest.TestCase):
    def test_registration_requires_explicit_first_time_authorization(self) -> None:
        source = REGISTER.read_text(encoding="utf-8")
        self.assertIn("ConfirmDedicatedCheckout", source)
        self.assertIn("if (-not (Test-Path $Sentinel", source)
        self.assertIn("if (-not $ConfirmDedicatedCheckout)", source)
        self.assertIn("refusing unattended registration", source)
        self.assertIn("remote get-url origin", source)

    def test_task_registration_does_not_pack_spaced_repo_path_into_schtasks_tr(self) -> None:
        source = REGISTER.read_text(encoding="utf-8")
        self.assertIn('"Everward Playtest"', source)
        self.assertIn("Register from XML instead", source)
        self.assertIn("ConvertTo-XmlText", source)
        self.assertIn("<Arguments>$EscapedArguments</Arguments>", source)
        self.assertIn("<WorkingDirectory>$EscapedRepoRoot</WorkingDirectory>", source)
        self.assertIn("/XML $TaskXmlPath", source)
        self.assertNotIn("/TR $Action", source)

    def test_one_click_setup_is_the_explicit_authorization_path(self) -> None:
        source = SETUP.read_text(encoding="utf-8")
        self.assertIn("-ConfirmDedicatedCheckout", source)
        self.assertIn(r"%USERPROFILE%\Documents\Everward Playtest", source)

    def test_manual_harness_refreshes_only_an_already_authorized_checkout(self) -> None:
        source = HARNESS.read_text(encoding="utf-8")
        self.assertIn("everward-unattended-worker", source)
        self.assertIn("Test-Path $DedicatedSentinel", source)
        self.assertNotIn("-ConfirmDedicatedCheckout", source)

    def test_manual_harness_and_worker_share_a_build_mutation_lock(self) -> None:
        harness = HARNESS.read_text(encoding="utf-8")
        worker = WORKER.read_text(encoding="utf-8")
        self.assertIn("everward-manual-playtest.lock", harness)
        self.assertIn('MANUAL_LOCK_NAME = "everward-manual-playtest.lock"', worker)
        self.assertIn("manual_playtest_active", worker)
        self.assertIn("unreal_build_running", worker)

    def test_worker_syncs_the_exact_discovered_target_without_second_api_lookup(self) -> None:
        source = WORKER.read_text(encoding="utf-8")
        self.assertIn("def sync_to_target", source)
        self.assertIn("sync_to_target(repo_root, target, state_dir)", source)
        sync_body = source.split("def sync_to_target", 1)[1].split("def resolve_unreal_root", 1)[0]
        self.assertNotIn("latest_successful_main_sha", sync_body)

    def test_worker_preserves_phase2_observation_evidence_before_clean(self) -> None:
        source = WORKER.read_text(encoding="utf-8")
        self.assertIn("preserve_playtest_evidence", source)
        self.assertIn('"preserved-playtests"', source)
        sync_body = source.split("def sync_to_target", 1)[1].split("def resolve_unreal_root", 1)[0]
        self.assertLess(sync_body.index("preserve_playtest_evidence"), sync_body.index('git_output(repo_root, "clean", "-fd")'))

    def test_headless_smoke_is_required_before_new_exact_commit_cache(self) -> None:
        source = WORKER.read_text(encoding="utf-8")
        self.assertIn('LAST_PASS_MARKER = "last_passed_headless_smoke_commit.txt"', source)
        self.assertNotIn('(state_dir / "last_passed_commit.txt")', source)
        self.assertIn("def run_unreal_headless_smoke", source)
        self.assertIn('"run_unreal_headless_smoke.ps1"', source)
        self.assertIn('report["checks"]["unreal_headless_smoke"] = headless_smoke', source)

        run_body = source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]
        build_index = run_body.index("run_unreal_build(")
        smoke_index = run_body.index("run_unreal_headless_smoke(")
        marker_index = run_body.index("LAST_PASS_MARKER).write_text")
        self.assertLess(build_index, smoke_index)
        self.assertLess(smoke_index, marker_index)

    def test_cached_pass_requires_post_smoke_marker_semantics(self) -> None:
        source = WORKER.read_text(encoding="utf-8")
        run_body = source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]
        self.assertIn("full preflight, UBT build, and headless Unreal smoke", run_body)
        self.assertIn("read_last_pass(state_dir) == target", run_body)


if __name__ == "__main__":
    unittest.main()
