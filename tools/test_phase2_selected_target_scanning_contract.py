"""Selected-target scan wiring: portable source contract.

These tests are not UBT/native Unreal or packaged gameplay proof.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UNREAL = ROOT / "unreal/Source/Everward"
SIM = ROOT / "src/simulation/include/everward/simulation"


class SelectedTargetScanningContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.controller = (UNREAL / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        cls.adapter = (UNREAL / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        cls.adapter_h = (UNREAL / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        cls.hud = (UNREAL / "EverwardHUD.cpp").read_text(encoding="utf-8")
        cls.read_model = (UNREAL / "ProbeHUDReadModel.cpp").read_text(encoding="utf-8")
        cls.read_h = (UNREAL / "ProbeHUDReadModel.h").read_text(encoding="utf-8")
        cls.core = (SIM / "core.hpp").read_text(encoding="utf-8")
        cls.runtime = (SIM / "software_policy.hpp").read_text(encoding="utf-8")
        cls.save = (SIM / "save_data.hpp").read_text(encoding="utf-8")
        cls.native = (UNREAL / "Tests/EverwardG03NativeTests.cpp").read_text(encoding="utf-8")

    def test_enter_scan_carries_selected_physical_target(self):
        handler = self.controller.split("void AEverwardPlayerController::ExecutePrimarySystemAction()", 1)[1].split(
            "void AEverwardPlayerController::ExecuteSecondarySystemAction()", 1)[0]
        self.assertIn("Adapter->GetSelectedTargetStatus()", handler)
        self.assertIn("Selection.bHasSelection ? Selection.TargetId : FString()", handler)
        self.assertIn("Phase2ScanDurationSeconds", handler)
        self.assertNotIn("BootstrapScanTargetId", handler)
        self.assertIn('TEXT("no target selected")', self.adapter)
        self.assertIn('FName CommandId(TEXT("start_scan"))', self.adapter)

    def test_existing_scan_identity_and_cancel_are_preserved(self):
        for snippet in (
            "probe_.active_scan_target_id = target_id;",
            'throw std::runtime_error("scan already in progress");',
            "const std::string cancelled_target = probe_.active_scan_target_id;",
            "probe_.active_scan_target_id.clear();",
            "observe_active_scan_progress(seconds);",
        ):
            self.assertIn(snippet, self.core)
        self.assertIn("ActiveScanTargetId", self.adapter_h)
        self.assertIn("ScanRemainingSeconds", self.adapter_h)
        self.assertIn("CommandCancelScan()", self.controller)
        self.assertIn('"active_scan_target_id"', self.save)

    def test_power_rules_not_replaced_with_invented_reset(self):
        self.assertIn("if (!probe_.is_scanning || !probe_.can_scan)", self.core)
        self.assertIn("core_.cancel_scan();", self.runtime)
        self.assertIn("watts < kGeneration1MinimumSensorPowerW", self.runtime)

    def test_hint_names_bound_target_and_respects_selected_knowledge(self):
        self.assertIn("const FEverwardTargetKnowledgeStatus& Knowledge", self.read_h)
        self.assertIn("Telemetry.ActiveScanTargetId", self.read_model)
        self.assertIn("Telemetry.ScanRemainingSeconds", self.read_model)
        self.assertIn("Knowledge.Confidence < 0.999", self.read_model)
        self.assertIn("Target.TargetId != Mining.TargetId", self.read_model)
        self.assertIn("TargetSelection, TargetKnowledge, Mining, Capabilities", self.hud)
        self.assertNotIn("SCAN TARGET NONE SELECTED //", self.hud)

    def test_completed_discovery_uses_real_record_only(self):
        self.assertIn("Discoveries.FindByPredicate(", self.hud)
        self.assertIn("Observed->Classification.IsEmpty()", self.hud)
        self.assertIn("Observed->Confidence", self.hud)
        self.assertNotIn("LastScanConfidence = 0.92", self.hud)
        self.assertNotIn("SILICATE-RICH // IRON-BEARING MATERIAL", self.hud)

    def test_native_world_regression_is_present(self):
        self.assertIn("Everward.G03.Native.SelectedTargetScanBinding", self.native)
        for snippet in ("CommandSelectTarget(RefA)", "CommandSelectTarget(RefB)",
                        "ActiveScanTargetId", "CommandCancelScan()", "no target selected",
                        "GetSelectedTargetKnowledgeStatus()", "GetMiningStatus().bSurveyed"):
            self.assertIn(snippet, self.native)


if __name__ == "__main__":
    unittest.main()
