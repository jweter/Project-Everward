import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SMOKE = ROOT / "tools" / "run_unreal_headless_smoke.ps1"
WORKER = ROOT / "tools" / "everward_unattended_worker.py"


class HeadlessSmokeTimeoutContractTests(unittest.TestCase):
    def test_headless_smoke_enforces_bounded_cold_start_floor(self) -> None:
        source = SMOKE.read_text(encoding="utf-8")
        self.assertIn('[int]$TimeoutSeconds = 900', source)
        self.assertIn('$EffectiveTimeoutSeconds = [Math]::Max($TimeoutSeconds, 900)', source)
        self.assertIn('$Process.WaitForExit($EffectiveTimeoutSeconds * 1000)', source)

    def test_worker_timeout_exceeds_smoke_floor_with_cleanup_grace(self) -> None:
        worker = WORKER.read_text(encoding="utf-8")
        self.assertIn('timeout_seconds=930.0', worker)
        self.assertNotIn('timeout_seconds=330.0', worker)
        self.assertNotIn('timeout_seconds=180.0', worker)


if __name__ == "__main__":
    unittest.main()
