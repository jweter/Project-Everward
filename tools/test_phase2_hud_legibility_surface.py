from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2HudLegibilitySurfaceTests(unittest.TestCase):
    """Protect the readable-HUD Product Reality fix at the source boundary.

    Hosted CI cannot compile or screenshot the Unreal project, so these checks
    do not claim visual acceptance. They prevent the specific fixed-pixel,
    tiny-font failure found in the 2026-08-30 local build from silently
    returning before the next local UE pass.
    """

    def setUp(self) -> None:
        self.hud_h = (SOURCE / "EverwardHUD.h").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(
            encoding="utf-8"
        )
        self.controller_cpp = (SOURCE / "EverwardPlayerController.cpp").read_text(
            encoding="utf-8"
        )
        self.environment = (
            SOURCE / "EverwardPhase2TestEnvironment.cpp"
        ).read_text(encoding="utf-8")

    def test_live_hud_uses_viewport_scaling_medium_font_and_text_floor(self) -> None:
        self.assertIn("ResolveHudScale", self.hud_cpp)
        self.assertIn("Canvas->ClipX / HudReferenceWidth", self.hud_cpp)
        self.assertIn("Canvas->ClipY / HudReferenceHeight", self.hud_cpp)
        self.assertIn("GetMediumFont", self.hud_cpp)
        self.assertIn("MinimumHudTextScale", self.hud_cpp)
        self.assertIn("ReadableTextScale", self.hud_cpp)
        self.assertNotIn("nullptr, 0.56f", self.hud_cpp)

    def test_f1_opens_a_dedicated_large_control_reference(self) -> None:
        self.assertIn("void ToggleControlsReference();", self.hud_h)
        self.assertIn("bool bControlsReferenceVisible = false;", self.hud_h)
        self.assertIn("void DrawControlsReference();", self.hud_h)
        self.assertIn("EKeys::F1", self.controller_cpp)
        self.assertIn("ToggleControlsReference", self.controller_h)
        for group in (
            "FLIGHT + CAMERA",
            "SYSTEMS",
            "MANIPULATOR + MINING",
            "[F1] RETURN TO LIVE HUD",
        ):
            self.assertIn(group, self.hud_cpp)

    def test_live_hud_keeps_only_short_entry_points_visible(self) -> None:
        self.assertIn(
            "[F1] CONTROLS   [TAB] SYSTEMS   [M] MANIPULATOR", self.hud_cpp
        )
        self.assertNotIn(
            "[1/2] DEPLOY   [3] TOOL   [N] ARM   [4/5/6] JOINT   [,][.] TARGET",
            self.hud_cpp,
        )

    def test_storage_shows_exact_authoritative_mass_and_percentage(self) -> None:
        self.assertIn("STORAGE %.1f / %.1f KG  (%s)", self.hud_cpp)
        self.assertIn("Telemetry.StorageUsedKilograms", self.hud_cpp)
        self.assertIn("Telemetry.StorageCapacityKilograms", self.hud_cpp)

    def test_target_instruction_is_materially_larger_than_failed_baseline(self) -> None:
        match = re.search(r"ScanTargetLabel->SetWorldSize\(([0-9.]+)f\)", self.environment)
        self.assertIsNotNone(match)
        self.assertGreaterEqual(float(match.group(1)), 160.0)
        self.assertIn("BootstrapBodyCenterZMeters * 100.0 + 700.0", self.environment)

    def test_duplicate_rejection_notice_is_suppressed(self) -> None:
        self.assertIn("bRejectedCommandAlreadyVisible", self.hud_cpp)
        self.assertIn("LastCommand.Detail.Equals(AutomationNotice.Detail)", self.hud_cpp)


if __name__ == "__main__":
    unittest.main()
