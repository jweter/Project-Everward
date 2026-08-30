from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ScanToMiningSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.mining = (SIM / "include/everward/simulation/mining.hpp").read_text(encoding="utf-8")
        self.cmake = (SIM / "CMakeLists.txt").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.bridge = (SOURCE / "ProbeMiningBridge.cpp").read_text(encoding="utf-8")
        self.controller_tick = (SOURCE / "EverwardPlayerControllerInteractionTick.cpp").read_text(encoding="utf-8")
        self.pawn_h = (SOURCE / "EverwardProbePawn.h").read_text(encoding="utf-8")
        self.highlight = (SOURCE / "ProbeManipulatorHighlight.cpp").read_text(encoding="utf-8")
        self.environment = (SOURCE / "EverwardPhase2TestEnvironment.cpp").read_text(encoding="utf-8")

    def test_mining_truth_is_engine_independent_and_scan_gated(self) -> None:
        self.assertIn("class MiningSystem", self.mining)
        self.assertIn("scan target first", self.mining)
        self.assertIn("kGeneration1ToolWorkingReachM", self.mining)
        self.assertIn("storage is full", self.mining)
        self.assertIn("ManipulatorRig", self.mining)
        self.assertNotIn("CoreMinimal.h", self.mining)
        self.assertNotIn("USTRUCT", self.mining)

    def test_cmake_runs_mining_behavior_tests(self) -> None:
        self.assertIn("mining_tests.cpp", self.cmake)
        self.assertIn("everward_mining_tests", self.cmake)

    def test_unreal_bridge_consumes_scan_truth_and_physical_arm_state(self) -> None:
        self.assertIn("FEverwardMiningStatus", self.adapter_h)
        self.assertIn("CommandMineBootstrapTarget", self.adapter_h)
        self.assertIn("LastScanLifecycleNotice.bCompleted", self.bridge)
        self.assertIn("Mining.mine_once", self.bridge)
        self.assertIn("*Manipulators", self.bridge)
        self.assertIn("Snapshot.storage_capacity_kg", self.bridge)

    def test_accepted_mining_routes_mass_through_authoritative_storage(self) -> None:
        # Extracted mass must land in Core's authoritative storage_used_kg,
        # the same field the systems-panel HUD's STORAGE readout reads
        # (ProbeSimulationAdapter.cpp: Telemetry.StorageUsedKilograms =
        # Snapshot.storage_used_kg). Accumulating it only in an
        # adapter-local counter would leave that HUD readout at zero even
        # while this mining status widget reports material recovered.
        self.assertIn("Core->add_stored_material_kg(Result.extracted_kg)", self.bridge)
        accepted_branch = self.bridge.split("if (Result.accepted)", 1)[1]
        self.assertIn(
            "Core->add_stored_material_kg(Result.extracted_kg)",
            accepted_branch.split("}", 1)[0],
        )
        # existing_storage_kg passed into mine_once must be the authoritative
        # field alone: once mined mass is routed into storage_used_kg above,
        # also adding the adapter-local BootstrapExtractedMaterialKilograms
        # counter on top of it would double-count already-stored material.
        self.assertIn("Snapshot.storage_used_kg,", self.bridge)
        self.assertNotIn(
            "Snapshot.storage_used_kg + BootstrapExtractedMaterialKilograms", self.bridge
        )

    def test_mining_control_is_discoverable_and_global_in_current_slice(self) -> None:
        self.assertIn("WasInputKeyJustPressed(EKeys::G)", self.controller_tick)
        self.assertIn("CommandMineBootstrapTarget", self.controller_tick)
        self.assertIn("UNSURVEYED RESOURCE BODY", self.environment)
        self.assertIn("[G] MINE", self.environment)
        self.assertIn("DepositRemainingKilograms", self.environment)

    def test_selected_arm_joint_has_world_space_visual_feedback(self) -> None:
        self.assertIn("SetManipulatorSelectionHighlight", self.pawn_h)
        self.assertIn("SetManipulatorSelectionHighlight", self.controller_tick)
        self.assertIn("SelectionHighlightColor", self.highlight)
        self.assertIn("EEverwardManipulatorJoint::Shoulder", self.highlight)
        self.assertIn("EEverwardManipulatorJoint::Elbow", self.highlight)
        self.assertIn("EEverwardManipulatorJoint::Wrist", self.highlight)
        self.assertIn("SetComponentColor(Shoulder", self.highlight)
        self.assertIn("SetComponentColor(Forearm", self.highlight)
        self.assertIn("SetComponentColor(ToolHead", self.highlight)


if __name__ == "__main__":
    unittest.main()
