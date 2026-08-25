"""Static regression checks for the Phase 2 capability-driven HUD contract.

Portable CI cannot build the Unreal module, so these checks protect the source-
level architecture that matters most: telemetry and installed capabilities come
through UProbeSimulationAdapter, the HUD reads rather than owns simulation
truth, contextual UI is wired by the Everward player controller, and safety-
critical flight state remains visible/actionable outside contextual subsystem UI.
"""

from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
ADAPTER_HEADER = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.h"
ADAPTER_SOURCE = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.cpp"
HUD_SOURCE = ROOT / "unreal/Source/Everward/EverwardHUD.cpp"
CONTROLLER_SOURCE = ROOT / "unreal/Source/Everward/EverwardPlayerController.cpp"
GAMEMODE_SOURCE = ROOT / "unreal/Source/Everward/EverwardGameMode.cpp"


class Phase2HudSourceContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.adapter_header = ADAPTER_HEADER.read_text(encoding="utf-8")
        self.adapter_source = ADAPTER_SOURCE.read_text(encoding="utf-8")
        self.hud = HUD_SOURCE.read_text(encoding="utf-8")
        self.controller = CONTROLLER_SOURCE.read_text(encoding="utf-8")
        self.game_mode = GAMEMODE_SOURCE.read_text(encoding="utf-8")

    def test_adapter_exposes_telemetry_and_installed_capabilities(self) -> None:
        self.assertIn("FEverwardProbeTelemetry", self.adapter_header)
        self.assertIn("FEverwardProbeCapability", self.adapter_header)
        self.assertIn("GetProbeTelemetry() const", self.adapter_header)
        self.assertIn("GetInstalledCapabilities() const", self.adapter_header)
        self.assertIn("Core->snapshot()", self.adapter_source)

    def test_capabilities_describe_control_and_automation_surfaces(self) -> None:
        self.assertIn("bSupportsManualControl", self.adapter_header)
        self.assertIn("bSupportsAutomation", self.adapter_header)
        for capability in ("propulsion", "sensors", "computation", "thermal"):
            self.assertIn(f'TEXT("{capability}")', self.adapter_source)

    def test_hud_reads_adapter_without_calling_simulation_core(self) -> None:
        self.assertIn("GetProbeTelemetry()", self.hud)
        self.assertIn("GetInstalledCapabilities()", self.hud)
        self.assertNotIn("SimulationCore", self.hud)
        self.assertNotIn("everward/simulation", self.hud)

    def test_hud_is_contextual_instead_of_permanently_expanded(self) -> None:
        self.assertIn("if (!bSystemsExpanded)", self.hud)
        self.assertIn("ToggleSystemsPanel()", self.hud)
        self.assertIn("SYSTEMS / CONTROL", self.hud)
        self.assertIn("MANUAL CONTROL", self.hud)
        self.assertIn("AUTOMATION API", self.hud)

    def test_controller_binds_contextual_system_navigation(self) -> None:
        self.assertIn("EKeys::Tab", self.controller)
        self.assertIn("EKeys::RightBracket", self.controller)
        self.assertIn("EKeys::LeftBracket", self.controller)
        self.assertIn("AEverwardHUD", self.controller)

    def test_spacebar_brake_is_global_not_propulsion_panel_gated(self) -> None:
        self.assertIn("EKeys::SpaceBar", self.controller)
        match = re.search(
            r"void AEverwardPlayerController::StopPropulsion\(\)\s*\{(?P<body>.*?)\n\}",
            self.controller,
            flags=re.DOTALL,
        )
        self.assertIsNotNone(match)
        body = match.group("body") if match is not None else ""
        self.assertIn("CommandSetVelocityMetersPerSecond(FVector::ZeroVector)", body)
        self.assertNotIn("IsSystemsPanelExpanded", body)
        self.assertNotIn("GetSelectedCapabilityId", body)
        self.assertNotIn('TEXT("propulsion")', body)

    def test_moving_probe_gets_persistent_local_flight_readout(self) -> None:
        self.assertIn("SpeedMetersPerSecond > 0.01", self.hud)
        self.assertIn("InverseTransformVectorNoScale", self.hud)
        self.assertIn("FLIGHT  %.2f m/s   [SPACE] BRAKE", self.hud)
        self.assertIn("FWD %+0.2f   RIGHT %+0.2f   UP %+0.2f m/s", self.hud)
        flight_pos = self.hud.index("FLIGHT  %.2f m/s")
        compact_return_pos = self.hud.index("if (!bSystemsExpanded)")
        self.assertLess(flight_pos, compact_return_pos)

    def test_game_mode_installs_hud_and_controller(self) -> None:
        self.assertIn("HUDClass = AEverwardHUD::StaticClass()", self.game_mode)
        self.assertIn("PlayerControllerClass = AEverwardPlayerController::StaticClass()", self.game_mode)


if __name__ == "__main__":
    unittest.main()
