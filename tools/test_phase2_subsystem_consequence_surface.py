from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal" / "Source" / "Everward"
RUNTIME = ROOT / "src" / "simulation" / "include" / "everward" / "simulation" / "software_policy.hpp"


class Phase2SubsystemConsequenceSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.hud = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")
        self.runtime = RUNTIME.read_text(encoding="utf-8")

    def test_sensor_power_floor_is_authoritative_and_recoverable(self) -> None:
        self.assertIn("kGeneration1MinimumSensorPowerW = 50.0", self.runtime)
        self.assertIn("sensors below minimum operating power", self.runtime)
        self.assertIn("watts < kGeneration1MinimumSensorPowerW", self.runtime)
        self.assertIn("core_.cancel_scan()", self.runtime)
        self.assertIn("ProbeRuntime::kGeneration1MinimumSensorPowerW", self.adapter_cpp)
        self.assertIn("BELOW MINIMUM POWER // NEED %.0f W", self.adapter_cpp)

    def test_capability_rows_explain_state_instead_of_only_showing_locked(self) -> None:
        self.assertIn("MinimumOperatingPowerWatts", self.adapter_h)
        self.assertIn("StatusReason", self.adapter_h)
        self.assertIn("Capability.StatusReason", self.hud)
        # The readable-HUD pass separates these formerly packed values into
        # two rows so the status reason and minimum-power requirement remain
        # legible at normal viewing distance.
        self.assertIn("STATUS %s", self.hud)
        self.assertIn("MINIMUM %.0f W", self.hud)
        self.assertIn("ENERGY DEPLETED", self.adapter_cpp)
        self.assertIn("THERMAL LOCKOUT", self.adapter_cpp)
        self.assertIn("HARDWARE FAILURE", self.adapter_cpp)

    def test_rejected_commands_are_promoted_to_brief_global_feedback(self) -> None:
        self.assertIn("COMMAND REJECTED // %s", self.hud)
        self.assertIn("CommandBannerExpiresAtWorldSeconds", self.hud)
        self.assertIn("!LastCommand.bAccepted", self.hud)

    def test_automation_reports_what_changed_and_why(self) -> None:
        self.assertIn("AUTOMATION: ", self.runtime)
        self.assertIn(" W -> ", self.runtime)
        self.assertIn("energy reserve below ", self.runtime)
        self.assertIn("GetLastAutomationNotice", self.adapter_h)
        self.assertIn("DomainEventType::PolicyRuleTriggered", self.adapter_cpp)
        self.assertIn("AutomationNotice.Detail", self.hud)
        self.assertIn("LAST AUTOMATION", self.hud)

    def test_canonical_startup_can_demonstrate_the_loop_immediately(self) -> None:
        self.assertIn(
            "runtime.core_.allocate_power(PowerSubsystem::Sensors, kGeneration1MinimumSensorPowerW)",
            self.runtime,
        )
        self.assertIn(
            "PowerSubsystem::Computation,\n            kGeneration1MinimumPolicyComputationPowerW",
            self.runtime,
        )


if __name__ == "__main__":
    unittest.main()
