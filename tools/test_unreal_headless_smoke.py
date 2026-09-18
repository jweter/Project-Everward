from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "run_unreal_headless_smoke.ps1"


class UnrealHeadlessSmokeTests(unittest.TestCase):
    def test_smoke_is_headless_and_non_rendering(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("UnrealEditor-Cmd.exe", source)
        self.assertIn('"-NullRHI"', source)
        self.assertIn('"-Unattended"', source)
        self.assertIn('"-NoSound"', source)
        self.assertIn('"-NoP4"', source)
        self.assertNotIn("Build.bat", source)

    def test_smoke_binds_logs_to_exact_commit_and_fails_closed(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("git -C $RepoRoot rev-parse HEAD", source)
        self.assertIn("headless-smoke-$GitCommit.log", source)
        self.assertIn("WaitForExit", source)
        self.assertIn("Headless Unreal smoke timed out", source)
        self.assertIn("Headless Unreal smoke failed with exit code", source)
        self.assertIn("RedirectStandardOutput", source)
        self.assertIn("RedirectStandardError", source)
        self.assertIn("stdout: $StdoutPath; stderr: $StderrPath", source)

    def test_smoke_does_not_claim_product_reality(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertNotIn("PRODUCT_REALITY_VERIFIED", source)
        self.assertNotIn("Product Reality PASS", source)


if __name__ == "__main__":
    unittest.main()
