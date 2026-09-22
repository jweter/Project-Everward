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
        self.assertIn('"-NoAutoSDK"', source)
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
        self.assertIn("taskkill.exe /PID $Process.Id /T /F", source)

    def test_smoke_uses_deterministic_engine_exit_hook(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertIn('-TestExit=`"Automation Test Queue Empty`"', source)
        self.assertNotIn('-ExecCmds=`"quit`"', source)

    def test_smoke_requires_deterministic_evidence_not_zero_exit_alone(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertIn("headless_smoke_complete", source)
        self.assertIn('$Session.status -eq "complete"', source)
        self.assertIn("$Process.ExitCode -ne 0 -and -not $SessionComplete", source)
        self.assertIn("did not produce complete deterministic evidence", source)
    def test_smoke_does_not_claim_product_reality(self) -> None:
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertNotIn("PRODUCT_REALITY_VERIFIED", source)
        self.assertNotIn("Product Reality PASS", source)


if __name__ == "__main__":
    unittest.main()


def test_smoke_has_deterministic_runtime_exit_after_evidence_capture() -> None:
    source = SCRIPT.read_text(encoding="utf-8")
    recorder = (ROOT / "unreal/Source/Everward/PlaytestRecorderActor.cpp").read_text(encoding="utf-8")
    header = (ROOT / "unreal/Source/Everward/PlaytestRecorderActor.h").read_text(encoding="utf-8")
    self_exit_flag = "EverwardHeadlessSmokeSeconds=10"
    assert self_exit_flag in source
    assert "EverwardHeadlessSmokeSeconds=" in recorder
    assert "headless_smoke_complete" in recorder
    assert "FPlatformMisc::RequestExit(false)" in recorder
    assert "HeadlessSmokeExitSeconds = 0.0" in header

