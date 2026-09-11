from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"
DOCS = ROOT / "docs"


class ControlledDescentPlayerLoopSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.tick_cpp = (SOURCE / "EverwardPlayerControllerInteractionTick.cpp").read_text(encoding="utf-8")
        self.descent_cpp = (SOURCE / "EverwardPlayerControllerDescent.cpp").read_text(encoding="utf-8")
        self.autopilot_cpp = (SOURCE / "EverwardPlayerControllerAutopilot.cpp").read_text(encoding="utf-8")
        self.mining_cpp = (SOURCE / "EverwardPlayerControllerMiningControls.cpp").read_text(encoding="utf-8")
        self.plan = (DOCS / "PHASE2_VERTICAL_SLICE_PLAN.md").read_text(encoding="utf-8")
        self.test_doc = (DOCS / "PHASE2_SURFACE_DESCENT_COMMAND_TEST.md").read_text(encoding="utf-8")

    def test_controller_declares_the_engage_advance_cancel_loop_and_tunables(self) -> None:
        for symbol in (
            "void ToggleControlledDescent();",
            "void AdvanceControlledDescent(float DeltaSeconds);",
            "void CancelControlledDescent(bool bStopVelocity, bool bShowMessage);",
            "bool bControlledDescentEngaged = false;",
        ):
            self.assertIn(symbol, self.controller_h)
        for tunable in (
            "ControlledDescentMaxDescentSpeedMetersPerSecond",
            "ControlledDescentMaxTangentialSpeedMetersPerSecond",
            "ControlledDescentMinimumClearanceMeters",
            "ControlledDescentFullSpeedAltitudeMeters",
            "ControlledDescentTouchdownSpeedMetersPerSecond",
        ):
            self.assertIn(tunable, self.controller_h)
            # Every EditAnywhere tuning knob must actually reach the command,
            # not be stranded once declared (the same rule
            # test_jose_autopilot_source_contract.py enforces for José).
            self.assertIn(tunable, self.descent_cpp)

    def test_descent_loop_governs_live_velocity_through_existing_boundaries(self) -> None:
        # Controlled descent is a velocity governor, not a destination
        # autopilot: it must re-read the probe's own current velocity every
        # step rather than commanding a fixed/hardcoded direction.
        self.assertIn("GetProbeTelemetry().VelocityMetersPerSecond", self.descent_cpp)
        self.assertIn("GetControlledDescentVelocityCommand", self.descent_cpp)
        self.assertIn("CommandSetControlledDescentVelocityMetersPerSecond", self.descent_cpp)
        self.assertIn("CommandSetVelocityMetersPerSecond(FVector::ZeroVector)", self.descent_cpp)
        self.assertNotIn("FMath::Clamp", self.descent_cpp)
        self.assertNotIn("SetActorLocation", self.descent_cpp)
        self.assertNotIn("Teleport", self.descent_cpp)

    def test_c_engages_and_manual_translation_releases_controlled_descent(self) -> None:
        self.assertIn("WasInputKeyJustPressed(EKeys::C)", self.tick_cpp)
        self.assertIn("ToggleControlledDescent", self.tick_cpp)
        self.assertIn("AdvanceControlledDescent(DeltaSeconds)", self.tick_cpp)
        self.assertIn("bManualTranslationRequested && bControlledDescentEngaged", self.tick_cpp)
        self.assertIn("CancelControlledDescent(false, true)", self.tick_cpp)

    def test_controlled_descent_does_not_compete_with_jose_or_mining_auto_approach(self) -> None:
        self.assertIn("CancelControlledDescent(false, false);", self.autopilot_cpp)
        self.assertIn("CancelJoseTakeTheWheel(false, false);", self.descent_cpp)
        self.assertIn("bAutoApproachMiningTarget = false;", self.descent_cpp)
        self.assertIn("CancelControlledDescent(true, false);", self.tick_cpp)

    def test_engage_fails_closed_with_no_registered_planetary_body(self) -> None:
        self.assertIn("bHasResult", self.descent_cpp)
        self.assertIn("registered planetary body", self.descent_cpp)

    def test_product_reality_hud_readout_is_discoverable(self) -> None:
        self.assertIn("CONTROLLED DESCENT", self.tick_cpp)
        self.assertIn("[C] ENGAGE", self.tick_cpp)

    def test_docs_reflect_the_wired_player_loop(self) -> None:
        self.assertIn("EverwardPlayerControllerDescent.cpp", self.plan + self.test_doc)
        self.assertIn("mutually exclusive", self.plan + self.test_doc)


if __name__ == "__main__":
    unittest.main()
