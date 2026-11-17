"""Issue #384: portable source contract for the first compact HUD slice.

This does not substitute for an UnrealBuildTool compile or game captures.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "unreal/Source/Everward"

class CompactHudContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model_h = (SRC / "ProbeHUDReadModel.h").read_text(encoding="utf-8")
        cls.model_cpp = (SRC / "ProbeHUDReadModel.cpp").read_text(encoding="utf-8")
        cls.hud_h = (SRC / "EverwardHUD.h").read_text(encoding="utf-8")
        cls.hud_cpp = (SRC / "EverwardHUD.cpp").read_text(encoding="utf-8")

    def test_model_reads_existing_authority_not_fake_hull(self):
        self.assertIn("FromAuthoritativeState", self.model_h)
        self.assertIn("FEverwardProbeTelemetry", self.model_h)
        self.assertIn("FEverwardTargetSelectionStatus", self.model_h)
        self.assertIn("SafeFraction", self.model_cpp)
        self.assertIn("EnergyCapacityJoules", self.model_cpp)
        self.assertIn("StorageCapacityKilograms", self.model_cpp)
        for subsystem in ("SensorsIntegrity", "PropulsionIntegrity", "ComputationIntegrity", "ThermalIntegrity"):
            self.assertIn(subsystem, self.model_cpp)
        self.assertNotIn("HullIntegrityPercent", self.model_h + self.model_cpp)
        self.assertIn("CoreTemperatureKelvin", self.model_h)
        self.assertIn("RuntimeFixItStatus", self.model_cpp)
        self.assertNotIn("IsFixItActive", self.model_cpp)
        self.assertNotIn("everward/simulation", self.model_cpp)

    def test_compact_target_preserves_numeric_range_and_identifier_suffix(self):
        # The selected-target card prints numeric surface range separately.
        # Only the identifier is abbreviated (preserving its suffix).
        self.assertIn("TargetSelection.bHasSelection", self.hud_cpp)
        self.assertIn("AbbreviateTargetIdForCompactRow(TargetSelection.TargetId, 29)", self.hud_cpp)
        self.assertIn("TargetId.Right(3)", self.hud_cpp)
        self.assertIn('TEXT("SURFACE RANGE %.1f M")', self.hud_cpp)
        self.assertIn("TargetSelection.SurfaceRangeMeters", self.hud_cpp)
        self.assertIn("TargetKnowledgeLine(TargetSelection, TargetKnowledge)", self.hud_cpp)
        self.assertNotIn('TruncatedPanelLine(FString::Printf(TEXT("TARGET %s // SURFACE RANGE', self.hud_cpp)

    def test_default_hud_is_compact_and_detail_on_demand(self):
        self.assertIn("bManipulatorPanelExpanded = false", self.hud_h)
        self.assertIn("FEverwardHUDReadModel::FromAuthoritativeState", self.hud_cpp)
        self.assertIn("GetFixItStatusSummary()", self.hud_cpp)
        self.assertIn("if (!bSystemsExpanded)", self.hud_cpp)
        self.assertIn("const float ExpandedHeight", self.hud_cpp)
        self.assertIn("WEAKEST %.0f%%", self.hud_cpp)
        self.assertIn("SURFACE RANGE", self.hud_cpp)
        self.assertIn("FixItStatus", self.hud_cpp)
        self.assertIn("return;", self.hud_cpp[self.hud_cpp.index("if (!bSystemsExpanded)"):self.hud_cpp.index("const float ExpandedHeight")])
        self.assertIn("GetProbeTelemetry()", self.hud_cpp)
        self.assertNotIn("everward/simulation", self.hud_cpp)

if __name__ == "__main__":
    unittest.main()
