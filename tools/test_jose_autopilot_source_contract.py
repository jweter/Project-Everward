from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"
DOCS = ROOT / "docs"
SIMULATION_INCLUDE = ROOT / "src/simulation/include/everward/simulation"
SIMULATION_TESTS = ROOT / "src/simulation/tests"


class JoseAutopilotSourceContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.tick_cpp = (SOURCE / "EverwardPlayerControllerInteractionTick.cpp").read_text(encoding="utf-8")
        self.autopilot_cpp = (SOURCE / "EverwardPlayerControllerAutopilot.cpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.target_bridge_cpp = (SOURCE / "ProbeTargetSelectionBridge.cpp").read_text(encoding="utf-8")
        self.playtesting = (DOCS / "PLAYTESTING.md").read_text(encoding="utf-8")
        self.design = (DOCS / "JOSE_TAKE_THE_WHEEL.md").read_text(encoding="utf-8")
        self.jose_autopilot_hpp = (SIMULATION_INCLUDE / "jose_autopilot.hpp").read_text(encoding="utf-8")
        self.cmake_lists = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_player_facing_feature_name_and_non_reference_note_are_preserved(self) -> None:
        self.assertIn("José Take the Wheel", self.design)
        self.assertIn("not intended as a reference to Jesus", self.design)
        self.assertIn("any religious figure", self.design)
        self.assertIn("any real person", self.design)
        self.assertIn("any pre-existing fictional character", self.design)

    def test_autopilot_uses_existing_authoritative_target_and_velocity_boundaries(self) -> None:
        self.assertIn("GetSelectedTargetStatus", self.autopilot_cpp)
        self.assertIn("GetStaticBodyPositionMeters", self.autopilot_cpp)
        self.assertIn("CommandSetVelocityMetersPerSecond", self.autopilot_cpp)
        self.assertNotIn("SetActorLocation", self.autopilot_cpp)
        self.assertNotIn("Teleport", self.autopilot_cpp)
        # Issue #239: the controller only configures the destination/tunables
        # via SetJoseAutopilotGovernorEngaged() here -- it no longer calls
        # GetJoseGuidanceCommand() or issues the resulting velocity command
        # itself, since doing so once per render frame made the guidance
        # decision's cadence depend on frame rate rather than the
        # simulation's tick sequence (the same defect #238 fixed for
        # controlled hover).
        self.assertIn("SetJoseAutopilotGovernorEngaged", self.autopilot_cpp)
        self.assertNotIn("GetJoseGuidanceCommand", self.autopilot_cpp)

    def test_autopilot_governor_correction_runs_once_per_fixed_step_not_per_frame(self) -> None:
        # Issue #239 regression protection, mirroring
        # test_controlled_hover_player_loop_surface.py's hover-side test: the
        # guidance decision and velocity command must be re-evaluated from
        # inside UProbeSimulationAdapter::TickComponent()'s own fixed-step
        # accumulator loop -- not from APlayerController::Tick() once per
        # render frame.
        adapter_h = self.adapter_h
        adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")

        self.assertIn("void SetJoseAutopilotGovernorEngaged(", adapter_h)
        self.assertIn("void AdvanceJoseAutopilotGovernorFixedStep();", adapter_h)
        self.assertIn("GetJoseGuidanceCommand", self.target_bridge_cpp)
        self.assertIn(
            "void UProbeSimulationAdapter::AdvanceJoseAutopilotGovernorFixedStep()", self.target_bridge_cpp
        )

        tick_component_start = adapter_cpp.index("::TickComponent(")
        sync_call = adapter_cpp.index("SyncOwnerTransformFromSimulation();", tick_component_start)
        tick_component_body = adapter_cpp[tick_component_start:sync_call]
        self.assertIn("Core->advance_wall_ticks", tick_component_body)
        self.assertIn("AdvanceJoseAutopilotGovernorFixedStep();", tick_component_body)
        # The correction call must land after Core has advanced, so it reads
        # the freshly-stepped pose rather than a stale one.
        self.assertLess(
            tick_component_body.index("Core->advance_wall_ticks"),
            tick_component_body.index("AdvanceJoseAutopilotGovernorFixedStep();"),
        )

    def test_cancel_jose_stops_the_fixed_step_governor_immediately(self) -> None:
        self.assertIn("SetJoseAutopilotGovernorEngaged(false", self.autopilot_cpp)

    def test_terminal_outcomes_stop_velocity_synchronously_in_the_fixed_step(self) -> None:
        # Codex review on PR #245: the first version of this fix disengaged
        # the governor on a terminal outcome (selection changed / arrived /
        # destination unresolved or not found) but left the probe's last
        # commanded velocity active until the controller noticed the notice
        # change on a later render tick -- reintroducing a render-cadence-
        # dependent stop inside the very PR meant to remove one. The zero-
        # velocity command must be issued synchronously inside
        # AdvanceJoseAutopilotGovernorFixedStep() itself, in the same fixed
        # step that detects the stop, not deferred to the controller.
        governor_start = self.target_bridge_cpp.index(
            "void UProbeSimulationAdapter::AdvanceJoseAutopilotGovernorFixedStep()"
        )
        governor_body = self.target_bridge_cpp[governor_start:]
        self.assertEqual(
            governor_body.count("CommandSetVelocityMetersPerSecond(FVector::ZeroVector)"),
            2,
            "expected one zero-velocity stop for the selection-changed branch and one for "
            "the Arrived/DestinationNotFound/DestinationUnresolved branch",
        )

        # The controller must not redundantly (and, worse, tardily) re-issue
        # the stop once it observes the notice -- that responsibility now
        # belongs entirely to the fixed-step governor above.
        self.assertNotIn("CancelJoseTakeTheWheel(true, false)", self.autopilot_cpp)

    def test_guidance_law_is_engine_independent_and_ctest_covered(self) -> None:
        # The approach-speed shaping and arrival decision must live in pure,
        # engine-independent code -- not recomputed in Unreal C++ -- so it is
        # deterministic and testable without launching Unreal, matching every
        # other Slice 7 sub-slice (target_selection.hpp, manipulator_reach.hpp).
        self.assertIn("jose_guidance_command", self.jose_autopilot_hpp)
        self.assertIn("JoseAutopilotConfig", self.jose_autopilot_hpp)
        self.assertIn("std::clamp", self.jose_autopilot_hpp)
        self.assertTrue((SIMULATION_TESTS / "jose_autopilot_tests.cpp").exists())
        self.assertIn("everward_jose_autopilot_tests", self.cmake_lists)

        # Unreal's autopilot controller must call into that math rather than
        # reimplementing it: no clamp/approach-speed arithmetic of its own.
        self.assertNotIn("FMath::Clamp", self.autopilot_cpp)
        self.assertIn("GetJoseGuidanceCommand", self.adapter_h)
        self.assertIn("jose_guidance_command_for_body", self.target_bridge_cpp)

    def test_autopilot_has_safe_surface_range_arrival_and_deceleration(self) -> None:
        self.assertIn("JoseArrivalSurfaceRangeMeters", self.controller_h)
        self.assertIn("JoseArrivalToleranceMeters", self.controller_h)
        self.assertIn("JoseApproachGainPerSecond", self.controller_h)
        self.assertIn("JoseCruiseSpeedMetersPerSecond", self.controller_h)
        # Every EditAnywhere tuning knob a designer can set per-instance must
        # still reach the guidance decision rather than being stranded once
        # the math moved into jose_autopilot.hpp.
        for tunable in (
            "JoseCruiseSpeedMetersPerSecond",
            "JoseArrivalSurfaceRangeMeters",
            "JoseArrivalToleranceMeters",
            "JoseApproachGainPerSecond",
        ):
            self.assertIn(tunable, self.autopilot_cpp)
        self.assertIn("CommandSetVelocityMetersPerSecond(FVector::ZeroVector)", self.autopilot_cpp)

    def test_y_engages_and_manual_translation_releases_the_wheel(self) -> None:
        self.assertIn("WasInputKeyJustPressed(EKeys::Y)", self.tick_cpp)
        self.assertIn("ToggleJoseTakeTheWheel", self.tick_cpp)
        for key in ("W", "S", "A", "D", "Q", "E"):
            self.assertIn(f"WasInputKeyJustPressed(EKeys::{key})", self.tick_cpp)
        self.assertIn("WasInputKeyJustPressed(EKeys::SpaceBar)", self.tick_cpp)
        self.assertIn("CancelJoseTakeTheWheel(false, true)", self.tick_cpp)

    def test_autopilot_and_mining_auto_approach_do_not_compete(self) -> None:
        self.assertIn("bAutoApproachMiningTarget = false", self.autopilot_cpp)
        self.assertIn("CancelJoseTakeTheWheel(true, false)", self.tick_cpp)
        self.assertIn("ToggleAutoApproachMiningTarget", self.tick_cpp)

    def test_product_reality_hud_and_playtest_contract_are_discoverable(self) -> None:
        self.assertIn("JOSÉ TAKE THE WHEEL", self.tick_cpp)
        self.assertIn("[T] SELECT DESTINATION", self.tick_cpp)
        self.assertIn("[Y] ENGAGE", self.tick_cpp)
        self.assertIn("José Take the Wheel validation", self.playtesting)
        self.assertIn("manual translation / immediate takeover", self.playtesting)
        self.assertIn("JOSE_TAKE_THE_WHEEL.md", self.playtesting)


if __name__ == "__main__":
    unittest.main()
