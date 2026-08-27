"""Static regression checks for the first Generation-1 software-policy surface.

Hosted CI cannot compile Unreal, so these checks protect the architectural
contract: the policy evaluator lives in the engine-independent simulation
layer, its actions call the same authoritative runtime power command used
manually, and the Unreal controller merely installs/clears policy through the
adapter.
"""

from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
POLICY = ROOT / "src/simulation/include/everward/simulation/software_policy.hpp"
ADAPTER_HEADER = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.h"
ADAPTER_SOURCE = ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.cpp"
CONTROLLER = ROOT / "unreal/Source/Everward/EverwardPlayerController.cpp"
HUD = ROOT / "unreal/Source/Everward/EverwardHUD.cpp"


class Phase2SoftwarePolicySurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.policy = POLICY.read_text(encoding="utf-8")
        self.adapter_header = ADAPTER_HEADER.read_text(encoding="utf-8")
        self.adapter_source = ADAPTER_SOURCE.read_text(encoding="utf-8")
        self.controller = CONTROLLER.read_text(encoding="utf-8")
        self.hud = HUD.read_text(encoding="utf-8")

    def test_generation_one_policy_is_small_and_compute_bounded(self) -> None:
        self.assertIn("kGeneration1MaxPolicyRules = 2", self.policy)
        self.assertIn("kGeneration1MinimumPolicyComputationPowerW = 25.0", self.policy)
        self.assertIn("power_allocated_computation_w", self.policy)
        self.assertIn("computation_operational", self.policy)

    def test_policy_actions_use_authoritative_manual_command(self) -> None:
        self.assertIn("allocate_power(rule.subsystem, rule.action_watts)", self.policy)
        self.assertIn("core_.allocate_power(subsystem, watts)", self.policy)
        self.assertNotIn("power_allocated_sensors_w = rule.action_watts", self.policy)
        self.assertNotIn("power_allocated_propulsion_w = rule.action_watts", self.policy)

    def test_adapter_exposes_policy_status_and_commands(self) -> None:
        self.assertIn("FEverwardSoftwarePolicyStatus", self.adapter_header)
        self.assertIn("GetSoftwarePolicyStatus() const", self.adapter_header)
        self.assertIn("CommandInstallBasicSurvivalPolicy", self.adapter_header)
        self.assertIn("CommandClearSoftwarePolicy", self.adapter_header)
        self.assertIn("ProbeRuntime::make_canonical_ev0001()", self.adapter_source)

    def test_manual_policy_controls_route_through_adapter(self) -> None:
        self.assertIn("CommandInstallBasicSurvivalPolicy()", self.controller)
        self.assertIn("CommandClearSoftwarePolicy()", self.controller)
        self.assertNotIn("SoftwarePolicyRule", self.controller)
        self.assertNotIn("SimulationCore", self.controller)

    def test_hud_explains_policy_and_compute_requirement(self) -> None:
        self.assertIn("GetSoftwarePolicyStatus()", self.hud)
        self.assertIn("INSTALL BASIC SURVIVAL", self.hud)
        self.assertIn("EXECUTOR: RUNNING", self.hud)
        self.assertIn("NEED >= %.0f W COMPUTE", self.hud)


if __name__ == "__main__":
    unittest.main()