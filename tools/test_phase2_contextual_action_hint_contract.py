"""Source contract for the first actionable-but-truthful compact HUD hint.

Portable CI does not compile Unreal; native and in-game checks remain separate.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "unreal/Source/Everward"


class ContextualActionHintContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model_h = (SRC / "ProbeHUDReadModel.h").read_text(encoding="utf-8")
        cls.model_cpp = (SRC / "ProbeHUDReadModel.cpp").read_text(encoding="utf-8")
        cls.hud = (SRC / "EverwardHUD.cpp").read_text(encoding="utf-8")
        cls.tick = (SRC / "EverwardPlayerControllerInteractionTick.cpp").read_text(encoding="utf-8")
        cls.controller = (SRC / "EverwardPlayerController.cpp").read_text(encoding="utf-8")
        cls.mining = (SRC / "ProbeMiningBridge.cpp").read_text(encoding="utf-8")

    def test_hints_are_presentation_not_new_simulation_authority(self):
        self.assertIn("FEverwardHUDActionHint", self.model_h)
        self.assertIn("FromAuthoritativeState(", self.model_h)
        for authority in ("FEverwardProbeTelemetry", "FEverwardTargetSelectionStatus",
                          "FEverwardMiningStatus", "FEverwardProbeCapability",
                          "FEverwardManipulatorArmState"):
            self.assertIn(authority, self.model_h)
        self.assertNotIn("Core->", self.model_cpp)
        self.assertNotIn("CommandMineBootstrapTarget(", self.model_cpp)

    def test_proposed_keys_match_real_player_controls(self):
        self.assertIn("WasInputKeyJustPressed(EKeys::G)", self.tick)
        self.assertIn("CommandMineBootstrapTarget()", self.tick)
        self.assertIn("WasInputKeyJustPressed(EKeys::P)", self.tick)
        self.assertIn("WasInputKeyJustPressed(EKeys::Seven)", self.tick)
        self.assertIn("BindKey(EKeys::T", self.controller)
        self.assertIn("BindKey(EKeys::F", self.controller)
        self.assertIn("BindKey(EKeys::Enter", self.controller)
        self.assertIn("!EverwardHUD->IsSystemsPanelExpanded()", self.controller)
        self.assertIn('FName(TEXT("sensors"))', self.controller)

    def test_scan_and_mining_hints_fail_closed(self):
        self.assertIn("Target.TargetId != Mining.TargetId", self.model_cpp)
        self.assertIn("!Mining.bSurveyed", self.model_cpp)
        self.assertIn("Capability.bOperational && Capability.bAvailable", self.model_cpp)
        self.assertIn("Arm.bToolAttached", self.model_cpp)
        self.assertIn("Mining.DepositRemainingKilograms <= 0.0", self.model_cpp)
        self.assertIn("Telemetry.StorageUsedKilograms >= Telemetry.StorageCapacityKilograms", self.model_cpp)
        self.assertIn('FName(TEXT("mine_bootstrap_target"))', self.mining)
        self.assertIn("Status.TargetId = AEverwardPhase2TestEnvironment::BootstrapScanTargetId", self.mining)
        self.assertNotIn("Target.SurfaceRangeMeters > Mining.ToolWorkingReachMeters", self.model_cpp)
        self.assertNotIn("[G] MINE TARGET", self.model_cpp)

    def test_existing_hud_row_reused_with_no_duplicate_prompt(self):
        self.assertIn("ContextHint = FEverwardHUDActionHint::FromAuthoritativeState", self.hud)
        self.assertIn("TruncatedPanelLine(ContextHint.Text, 49)", self.hud)
        self.assertIn('FString(TEXT("TARGET NONE"))', self.hud)
        self.assertNotIn('FString(TEXT("TARGET NONE // [T] SELECT"))', self.hud)
        self.assertIn("HudState.FixItStatus", self.hud)
        self.assertIn("const float ExpandedHeight", self.hud)


if __name__ == "__main__":
    unittest.main()
