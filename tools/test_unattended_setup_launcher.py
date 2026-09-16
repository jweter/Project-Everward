from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SETUP = ROOT / "tools/Setup_Everward_Unattended_Testing.bat"
REGISTER = ROOT / "tools/register_unattended_product_reality.ps1"


class UnattendedSetupLauncherTests(unittest.TestCase):
    def test_setup_targets_dedicated_playtest_checkout_and_latest_green_main(self) -> None:
        source = SETUP.read_text(encoding="utf-8")
        self.assertIn(r"%USERPROFILE%\Documents\Everward Playtest", source)
        self.assertIn("foundation.yml/runs?branch=main&status=success&event=push", source)
        self.assertIn("git checkout --detach", source)
        self.assertIn("register_unattended_product_reality.ps1", source)
        self.assertIn("-ConfirmDedicatedCheckout", source)

    def test_setup_configures_idle_reporting_without_launching_unreal(self) -> None:
        source = SETUP.read_text(encoding="utf-8")
        self.assertIn("auth login --web", source)
        self.assertIn("issue #243", source)
        self.assertIn("10 minutes of Windows idle time", source)
        self.assertNotIn("UnrealEditor.exe", source)
        self.assertIn("YOU ARE OUT OF THE ROUTINE TEST LOOP", source)

    def test_explicit_setup_starts_one_background_worker_run(self) -> None:
        setup = SETUP.read_text(encoding="utf-8")
        register = REGISTER.read_text(encoding="utf-8")
        self.assertIn("-RunOnceNow", setup)
        self.assertIn("[switch]$RunOnceNow", register)
        self.assertIn("schtasks.exe /Run /TN $TaskName", register)
        self.assertIn("first worker run is now running in the background", setup)
        # The on-demand start is opt-in to the explicit setup path; normal
        # playtest refreshes call the registration script without RunOnceNow.
        self.assertIn("if ($RunOnceNow)", register)


if __name__ == "__main__":
    unittest.main()
