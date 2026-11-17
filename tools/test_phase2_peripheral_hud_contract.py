"""Issue #384 world-first first layout pass: source integration contracts.

These tests cannot establish Unreal UBT compilation, screenshot legibility,
text clipping or gameplay feel; those require real product evidence.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
UNREAL = ROOT / "unreal/Source/Everward"
LAYOUT = UNREAL / "PeripheralHUDLayout.h"
HUD = UNREAL / "EverwardHUD.cpp"
NATIVE = UNREAL / "Tests/EverwardPeripheralHUDLayoutTest.cpp"
CONTROLLER = UNREAL / "EverwardPlayerController.cpp"


class PeripheralHUDSurfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout = LAYOUT.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")
        cls.native = NATIVE.read_text(encoding="utf-8")
        cls.controller = CONTROLLER.read_text(encoding="utf-8")

    def test_geometry_has_three_bounded_default_zones(self):
        for item in (
            "struct FEverwardPeripheralHUDLayout",
            "FBox2D Vitals",
            "FBox2D Target",
            "FBox2D Action",
            "static FEverwardPeripheralHUDLayout Compute",
            "ViewportWidth / 1920.0f",
            "ViewportHeight / 1080.0f",
            "0.80f, 1.75f",
        ):
            self.assertIn(item, self.layout)
        self.assertIn("FEverwardPeripheralHUDLayout::Compute(Canvas->ClipX, Canvas->ClipY)", self.hud)
        self.assertIn("Peripheral.Vitals.Min.Y", self.hud)
        self.assertIn("Peripheral.Target.Min.Y", self.hud)
        self.assertIn("Peripheral.Action.Min.Y", self.hud)

    def test_no_new_input_authority_or_misleading_hull_health(self):
        self.assertIn("HudState.WeakestSubsystemFraction", self.hud)
        self.assertIn("HudState.EnergyFraction", self.hud)
        self.assertIn("HudState.StorageFraction", self.hud)
        self.assertNotIn("HullIntegrityPercent", self.hud)
        self.assertNotIn("CommandStartScan(", self.hud)
        self.assertNotIn("CommandMineBootstrapTarget(", self.hud)
        self.assertIn("TargetSelection.SurfaceRangeMeters", self.hud)
        self.assertIn("TargetKnowledgeLine(TargetSelection, TargetKnowledge)", self.hud)
        self.assertIn("TruncatedPanelLine(ContextHint.Text, 48)", self.hud)

    def test_full_current_diagnostics_remain_accessible(self):
        self.assertIn("const float ExpandedHeight", self.hud)
        self.assertIn("MaterialInventoryLine(MaterialInventory)", self.hud)
        self.assertIn("DiscoveredTargetsLine(Discoveries)", self.hud)
        self.assertIn("GetRestorationObjectiveCycleStatus()", self.hud)
        self.assertIn("MANIPULATOR CONTROL", self.hud)
        for key in ("F1", "Tab", "M"):
            self.assertIn(f"EKeys::{key}", self.controller)
        self.assertNotIn("EKeys::F2", self.controller)

    def test_native_test_checks_center_and_720p_without_claiming_product_reality(self):
        for item in (
            '"Everward.G03.Native.PeripheralHUDLayout"',
            "FVector2D(1920.0f, 1080.0f)",
            "FVector2D(1280.0f, 720.0f)",
            "Viewport.X * 0.20f",
            "Viewport.Y * 0.30f",
            "Viewport.X * 0.80f",
            "Viewport.Y * 0.70f",
            "TestFalse",
            "return !HasAnyErrors();",
        ):
            self.assertIn(item, self.native)


if __name__ == "__main__":
    unittest.main()
