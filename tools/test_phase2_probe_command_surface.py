"""Static regression checks for the Phase 2 shared probe command surface.

Hosted CI cannot compile the Unreal module, so these tests protect the source-
level contract that manual controls and future automation share one adapter
command boundary while mechanical truth remains in src/simulation.
"""

from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
ADAPTER_HEADER = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.h"
ADAPTER_SOURCE = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.cpp"
HUD_SOURCE = ROOT / "unreal/Source/Everward/EverwardHUD.cpp"
CONTROLLER_SOURCE = ROOT / "unreal/Source/Everward/EverwardPlayerController.cpp"


class Phase2ProbeCommandSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.adapter_header = ADAPTER_HEADER.read_text(encoding="utf-8")
        self.adapter_source = ADAPTER_SOURCE.read_text(encoding="utf-8")
        self.hud = HUD_SOURCE.read_text(encoding="utf-8")
        self.controller = CONTROLLER_SOURCE.read_text(encoding="utf-8")

    def test_adapter_exposes_one_observable_command_surface(self) -> None:
        self.assertIn("FEverwardProbeCommandResult", self.adapter_header)
        self.assertIn("CommandSetVelocityMetersPerSecond", self.adapter_header)
        self.assertIn("CommandStartScan", self.adapter_header)
        self.assertIn("CommandCancelScan", self.adapter_header)
        self.assertIn("CommandAllocatePower", self.adapter_header)
        self.assertIn("GetLastCommandResult", self.adapter_header)

    def test_adapter_routes_commands_to_authoritative_simulation(self) -> None:
        self.assertIn("Core->set_velocity_mps", self.adapter_source)
        self.assertIn("Core->start_scan", self.adapter_source)
        self.assertIn("Core->cancel_scan", self.adapter_source)
        self.assertIn("Core->allocate_power", self.adapter_source)
        self.assertIn("catch (const std::exception& Error)", self.adapter_source)
        self.assertIn("RecordCommandResult", self.adapter_source)

    def test_legacy_velocity_wrapper_delegates_to_shared_command(self) -> None:
        wrapper = self.adapter_source.split(
            "void UProbeSimulationAdapter::SetProbeVelocityMetersPerSecond", 1
        )[1].split(
            "FEverwardProbeCommandResult UProbeSimulationAdapter::RecordCommandResult", 1
        )[0]
        self.assertIn("CommandSetVelocityMetersPerSecond", wrapper)
        self.assertNotIn("Core->set_velocity_mps", wrapper)

    def test_manual_controller_uses_adapter_commands_not_simulation_core(self) -> None:
        self.assertNotIn("SimulationCore", self.controller)
        self.assertNotIn("everward/simulation", self.controller)
        self.assertIn("Adapter->CommandStartScan", self.controller)
        self.assertIn("Adapter->CommandCancelScan", self.controller)
        self.assertIn("Adapter->CommandAllocatePower", self.controller)
        self.assertIn("Adapter->CommandSetVelocityMetersPerSecond", self.controller)

    def test_contextual_manual_controls_are_bound(self) -> None:
        for key in (
            "EKeys::Enter",
            "EKeys::BackSpace",
            "EKeys::PageUp",
            "EKeys::PageDown",
            "EKeys::Up",
            "EKeys::Down",
            "EKeys::SpaceBar",
        ):
            self.assertIn(key, self.controller)
        self.assertIn("IsSystemsPanelExpanded", self.controller)

    def test_hud_observes_command_and_active_scan_state(self) -> None:
        self.assertNotIn("SimulationCore", self.hud)
        self.assertNotIn("everward/simulation", self.hud)
        self.assertIn("GetLastCommandResult", self.hud)
        self.assertIn("bIsScanning", self.hud)
        self.assertIn("ScanRemainingSeconds", self.hud)
        self.assertIn("TotalPowerAllocatedWatts", self.hud)
        self.assertIn("CMD %s", self.hud)


if __name__ == "__main__":
    unittest.main()
