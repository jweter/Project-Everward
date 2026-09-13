from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORKER = ROOT / "tools/unattended/run_everward_windows_worker.ps1"
REGISTER = ROOT / "tools/unattended/register_everward_windows_worker.ps1"
BOOTSTRAP = ROOT / "tools/unattended/bootstrap_everward_windows_worker.ps1"
PUBLISHER = ROOT / "tools/unattended/publish_everward_worker_result.ps1"
SETUP_BAT = ROOT / "tools/unattended/Everward-Unattended-Setup.bat"
DOC = ROOT / "docs/UNATTENDED_WINDOWS_PRODUCT_REALITY.md"


class UnattendedWindowsWorkerContractTests(unittest.TestCase):
    def test_worker_is_exact_commit_fail_closed_and_local_state_only(self) -> None:
        source = WORKER.read_text(encoding="utf-8")
        self.assertIn('"Everward\\unattended-worker"', source)
        self.assertIn("git -C", source.replace("& $Git.Source -C", "git -C"))
        self.assertIn("status --porcelain", source)
        self.assertIn('result = "REVIEW_REQUIRED"', source)
        self.assertIn('product_reality_claimed = $false', source)
        self.assertIn("quality_preflight.py", source)
        self.assertIn("--full", source)
        self.assertIn("EverwardEditor Win64 Development", source)
        self.assertIn("worker.lock", source)
        self.assertIn("StaleLockHours", source)

    def test_registration_is_current_user_idle_and_uses_durable_bootstrap(self) -> None:
        source = REGISTER.read_text(encoding="utf-8")
        self.assertIn("/SC ONIDLE", source)
        self.assertIn("/I $IdleMinutes", source)
        self.assertNotIn("/RU SYSTEM", source)
        self.assertIn("worker-bootstrap.ps1", source)
        self.assertIn("Copy-Item -Force", source)
        self.assertIn("dedicated_checkout", source)

    def test_bootstrap_uses_separate_latest_main_checkout(self) -> None:
        source = BOOTSTRAP.read_text(encoding="utf-8")
        self.assertIn("https://github.com/jweter/Project-Everward.git", source)
        self.assertIn('$Branch = "main"', source)
        self.assertIn('"checkout"', source)
        self.assertIn("git clone", source.replace("& $Git.Source clone", "git clone"))
        self.assertIn("fetch --prune origin $Branch", source)
        self.assertIn('reset --hard "origin/$Branch"', source)
        self.assertIn("run_everward_windows_worker.ps1", source)
        self.assertIn("publish_everward_worker_result.ps1", source)

    def test_publisher_posts_only_changed_sanitized_summary_when_gh_authenticated(self) -> None:
        source = PUBLISHER.read_text(encoding="utf-8")
        self.assertIn("last_published_key.txt", source)
        self.assertIn("gh.exe", source)
        self.assertIn("auth status", source)
        self.assertIn("issue comment", source)
        self.assertIn("jweter/Project-Everward", source)
        self.assertIn("IssueNumber = 240", source)
        self.assertIn("engineering/build verification only", source)
        self.assertNotIn("log_tail", source)

    def test_desktop_setup_downloads_merged_main_registration_and_bootstrap(self) -> None:
        source = SETUP_BAT.read_text(encoding="utf-8")
        self.assertIn("raw.githubusercontent.com/jweter/Project-Everward/main/tools/unattended", source)
        self.assertIn("register_everward_windows_worker.ps1", source)
        self.assertIn("bootstrap_everward_windows_worker.ps1", source)
        self.assertIn("-IdleMinutes 10", source)
        self.assertIn("You do NOT need to launch Unreal", source)

    def test_document_preserves_human_only_for_experience_boundary(self) -> None:
        source = DOC.read_text(encoding="utf-8")
        self.assertIn("Zero humans for facts a computer can measure", source)
        self.assertIn("PASS **does not** mean", source)
        self.assertIn("scripted gameplay driver", source)
        self.assertIn("milestone-only human playtest", source)


if __name__ == "__main__":
    unittest.main()
