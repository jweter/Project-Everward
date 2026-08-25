from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
HUD_SOURCE = ROOT / "unreal/Source/Everward/EverwardHUD.cpp"
HUD_HEADER = ROOT / "unreal/Source/Everward/EverwardHUD.h"


class Phase2ScanDiscoverySurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.hud = HUD_SOURCE.read_text(encoding="utf-8")
        self.header = HUD_HEADER.read_text(encoding="utf-8")

    def test_completed_scan_is_persisted_and_visible(self) -> None:
        self.assertIn("bHasScanDiscovery", self.header)
        self.assertIn("bWasScanning", self.header)
        self.assertIn("SCAN COMPLETE  //  %s  //  DISCOVERY STORED", self.hud)
        self.assertIn("LAST DISCOVERY", self.hud)
        self.assertIn("ROCKY BODY / SURVEY REFERENCE", self.hud)
        self.assertIn("SILICATE-RICH // IRON-BEARING MATERIAL", self.hud)
        self.assertIn("CONFIDENCE %.0f%%  //  ACQUIRED T+%.1fs", self.hud)

    def test_cancelled_scan_does_not_create_discovery(self) -> None:
        self.assertIn('LastCommand.CommandId == FName(TEXT("cancel_scan"))', self.hud)
        self.assertIn("if (!bWasCancelled", self.hud)


if __name__ == "__main__":
    unittest.main()
