from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
REGISTER = ROOT / "tools/register_unattended_product_reality.ps1"
RUNNER = ROOT / "tools/run_unattended_product_reality.ps1"
PUBLISHER = ROOT / "tools/publish_unattended_product_reality.ps1"


class UnattendedProductRealityReportingTests(unittest.TestCase):
    def test_registration_uses_reporting_wrapper_and_idle_task(self) -> None:
        source = REGISTER.read_text(encoding="utf-8")
        self.assertIn("run_unattended_product_reality.ps1", source)
        self.assertIn("/XML $TaskXmlPath", source)
        self.assertIn("<IdleTrigger>", source)
        self.assertIn("<Duration>PT10M</Duration>", source)
        self.assertIn("<RunLevel>LeastPrivilege</RunLevel>", source)
        self.assertIn("<Command>powershell.exe</Command>", source)
        self.assertIn("<Arguments>$EscapedArguments</Arguments>", source)
        self.assertIn("<WorkingDirectory>$EscapedRepoRoot</WorkingDirectory>", source)
        self.assertIn("/Query /TN $TaskName", source)
        self.assertNotIn("/TR $Action", source)
        self.assertIn("issue #243", source)

    def test_runner_keeps_publication_separate_from_test_truth(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        self.assertIn("everward_unattended_worker.py", source)
        self.assertIn("publish_unattended_product_reality.ps1", source)
        self.assertIn("best-effort", source)
        self.assertIn("exit $WorkerExit", source)

    def test_publisher_updates_one_dedicated_issue_without_logs(self) -> None:
        source = PUBLISHER.read_text(encoding="utf-8")
        self.assertIn("IssueNumber = 243", source)
        self.assertIn("gh.exe", source)
        self.assertIn("auth status", source)
        self.assertIn("issue edit", source)
        self.assertIn("Gameplay/visual Product Reality claimed", source)
        self.assertNotIn("failure_tail", source)
        self.assertNotIn("repo_root", source)


if __name__ == "__main__":
    unittest.main()
