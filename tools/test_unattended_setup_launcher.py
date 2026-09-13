from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SETUP = ROOT / "tools/Setup_Everward_Unattended_Testing.bat"


class UnattendedSetupLauncherTests(unittest.TestCase):
    def test_setup_targets_dedicated_playtest_checkout_and_latest_green_main(self) -> None:
        source = SETUP.read_text(encoding="utf-8")
        self.assertIn(r"%USERPROFILE%\Documents\Everward Playtest", source)
        self.assertIn("foundation.yml/runs?branch=main&status=success&event=push", source)
        self.assertIn("git checkout --detach", source)
        self.assertIn("register_unattended_product_reality.ps1", source)

    def test_setup_configures_idle_reporting_without_launching_unreal(self) -> None:
        source = SETUP.read_text(encoding="utf-8")
        self.assertIn("gh auth login --web", source)
        self.assertIn("issue #243", source)
        self.assertIn("10 minutes of Windows idle time", source)
        self.assertNotIn("UnrealEditor.exe", source)
        self.assertIn("YOU ARE OUT OF THE ROUTINE TEST LOOP", source)


if __name__ == "__main__":
    unittest.main()
