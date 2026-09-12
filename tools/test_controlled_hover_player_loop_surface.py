from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"
DOCS = ROOT / "docs"


class ControlledHoverPlayerLoopSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.tick_cpp = (SOURCE / "EverwardPlayerControllerInteractionTick.cpp").read_text(encoding="utf-8")
        self.hover_cpp = (SOURCE / "EverwardPlayerControllerHover.cpp").read_text(encoding="utf-8")
        self.descent_cpp = (SOURCE / "EverwardPlayerControllerDescent.cpp").read_text(encoding="utf-8")
        self.autopilot_cpp = (SOURCE / "EverwardPlayerControllerAutopilot.cpp").read_text(encoding="utf-8")
        self.plan = (DOCS / "PHASE2_VERTICAL_SLICE_PLAN.md").read_text(encoding="utf-8")
        self.test_doc = (DOCS / "PHASE2_SURFACE_HOVER_COMMAND_TEST.md").read_text(encoding="utf-8")

    def test_controller_declares_the_engage_advance_cancel_loop_and_tunables(self) -> None:
        for symbol in (
            "void ToggleControlledHover();",
            "void AdvanceControlledHover(float DeltaSeconds);",
            "void CancelControlledHover(bool bStopVelocity, bool bShowMessage);",
            "bool bControlledHoverEngaged = false;",
        ):
            self.assertIn(symbol, self.controller_h)
        for tunable in (
            "ControlledHoverMaxTangentialSpeedMetersPerSecond",
            "ControlledHoverMinimumClearanceMeters",
            "ControlledHoverTargetAltitudeMeters",
            "ControlledHoverAltitudeGainPerSecond",
            "ControlledHoverMaxVerticalCorrectionSpeedMetersPerSecond",
        ):
            self.assertIn(tunable, self.controller_h)
            # Every EditAnywhere tuning knob must actually reach the command,
            # not be stranded once declared (the same rule
            # test_controlled_descent_player_loop_surface.py enforces for
            # controlled descent's own tunables).
            self.assertIn(tunable, self.hover_cpp)

    def test_hover_loop_governs_live_velocity_through_existing_boundaries(self) -> None:
        # Controlled hover is a velocity governor, not a destination
        # autopilot: it must re-read the probe's own current velocity every
        # step rather than commanding a fixed/hardcoded direction.
        self.assertIn("GetProbeTelemetry().VelocityMetersPerSecond", self.hover_cpp)
        self.assertIn("GetControlledHoverVelocityCommand", self.hover_cpp)
        self.assertIn("CommandSetControlledHoverVelocityMetersPerSecond", self.hover_cpp)
        self.assertIn("CommandSetVelocityMetersPerSecond(FVector::ZeroVector)", self.hover_cpp)
        self.assertNotIn("FMath::Clamp", self.hover_cpp)
        self.assertNotIn("SetActorLocation", self.hover_cpp)
        self.assertNotIn("Teleport", self.hover_cpp)

    def test_v_and_space_engage_and_release_controlled_hover(self) -> None:
        self.assertIn("WasInputKeyJustPressed(EKeys::V)", self.tick_cpp)
        self.assertIn("ToggleControlledHover", self.tick_cpp)
        self.assertIn("AdvanceControlledHover(DeltaSeconds)", self.tick_cpp)
        self.assertIn(
            "WasInputKeyJustPressed(EKeys::SpaceBar) && bControlledHoverEngaged", self.tick_cpp
        )
        self.assertIn("CancelControlledHover(false, true)", self.tick_cpp)

    def test_ordinary_translation_does_not_cancel_hover(self) -> None:
        # Unlike José/descent, hover's entire purpose is preserving the
        # player's tangential translation request while only the radial
        # component is governed toward the target altitude -- so ordinary
        # WASDQE trim must not disengage it, or lateral steering while
        # hovering would be impossible. Only V/SPACE release it.
        self.assertNotIn("bManualTranslationRequested && bControlledHoverEngaged", self.tick_cpp)

    def test_controlled_hover_does_not_compete_with_jose_descent_or_mining_auto_approach(self) -> None:
        self.assertIn("CancelControlledHover(false, false);", self.autopilot_cpp)
        self.assertIn("CancelControlledHover(false, false);", self.descent_cpp)
        self.assertIn("CancelJoseTakeTheWheel(false, false);", self.hover_cpp)
        self.assertIn("CancelControlledDescent(false, false);", self.hover_cpp)
        self.assertIn("bAutoApproachMiningTarget = false;", self.hover_cpp)
        self.assertIn("CancelControlledHover(true, false);", self.tick_cpp)

    def test_engage_fails_closed_with_no_registered_planetary_body(self) -> None:
        self.assertIn("bHasResult", self.hover_cpp)
        self.assertIn("registered planetary body", self.hover_cpp)

    def test_product_reality_hud_readout_is_discoverable(self) -> None:
        self.assertIn("CONTROLLED HOVER", self.tick_cpp)
        self.assertIn("[V] ENGAGE", self.tick_cpp)

    def test_docs_reflect_the_wired_player_loop(self) -> None:
        self.assertIn("EverwardPlayerControllerHover.cpp", self.plan + self.test_doc)
        self.assertIn("mutually exclusive", self.plan + self.test_doc)


if __name__ == "__main__":
    unittest.main()
