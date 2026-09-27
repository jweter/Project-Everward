from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
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
        self.assertIn("Get-FailureFingerprint", source)
        self.assertIn("sanitized failure fingerprint", source)
        self.assertNotIn("repo_root", source)


if __name__ == "__main__":
    unittest.main()


def test_publisher_avoids_encoding_sensitive_punctuation_and_ambiguous_variable_colon():
    script = (ROOT / "tools" / "publish_unattended_product_reality.ps1").read_text(encoding="utf-8")
    assert "—" not in script
    assert '"- **${Name}:** $Status"' in script


def _fingerprint_function_source() -> str:
    source = PUBLISHER.read_text(encoding="utf-8")
    start = source.index("function Get-FailureFingerprint")
    end = source.index("\nif (-not (Test-Path $Latest", start)
    return source[start:end]


@unittest.skipUnless(shutil.which("powershell.exe") or shutil.which("pwsh"), "PowerShell required")
class FailureFingerprintBehaviorTests(unittest.TestCase):
    def run_fingerprint(self, object_literal: str) -> str:
        shell = shutil.which("powershell.exe") or shutil.which("pwsh")
        assert shell is not None
        with tempfile.TemporaryDirectory() as temp:
            script = Path(temp) / "fingerprint.ps1"
            script.write_text(
                _fingerprint_function_source()
                + "\n$Check = "
                + object_literal
                + "\nGet-FailureFingerprint $Check\n",
                encoding="utf-8",
            )
            completed = subprocess.run(
                [shell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script)],
                text=True,
                capture_output=True,
                check=False,
            )
        self.assertEqual(completed.returncode, 0, completed.stderr)
        return completed.stdout.strip()

    def test_editor_failure_uses_nested_editor_exit_code(self) -> None:
        value = self.run_fingerprint(
            '[pscustomobject]@{ status="FAIL"; exit_code=0; '
            'failure_reason="editor_exited_during_sample_window"; '
            'startup=[pscustomobject]@{ exited_during_sample_window=$true; editor_exit_code=3 }; '
            'failure_tail="" }'
        )
        self.assertEqual(value, "editor_exited_during_sample_window|exit=3")

    def test_editor_failure_without_nested_code_does_not_publish_wrapper_zero(self) -> None:
        value = self.run_fingerprint(
            '[pscustomobject]@{ status="FAIL"; exit_code=0; '
            'failure_category="exited_before_startup"; '
            'startup=[pscustomobject]@{ exited_during_sample_window=$false; editor_exit_code=$null }; '
            'failure_tail="" }'
        )
        self.assertEqual(value, "exited_before_startup|exit=unknown")

    def test_non_editor_failure_keeps_command_exit_code(self) -> None:
        value = self.run_fingerprint(
            '[pscustomobject]@{ status="FAIL"; exit_code=17; '
            'failure_category=""; failure_reason=""; '
            'startup=[pscustomobject]@{ exited_during_sample_window=$false; editor_exit_code=$null }; '
            'failure_tail="cmake failed" }'
        )
        self.assertEqual(value, "cmake|exit=17")
