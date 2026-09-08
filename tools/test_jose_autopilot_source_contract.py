from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"
DOCS = ROOT / "docs"


class JoseAutopilotSourceContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.controller_h = (SOURCE / "EverwardPlayerController.h").read_text(encoding="utf-8")
        self.tick_cpp = (SOURCE / "EverwardPlayerControllerInteractionTick.cpp").read_text(encoding="utf-8")
        self.autopilot_cpp = (SOURCE / "EverwardPlayerControllerAutopilot.cpp").read_text(encoding="utf-8")
        self.playtesting = (DOCS / "PLAYTESTING.md").read_text(encoding="utf-8")
        self.design = (DOCS / "JOSE_TAKE_THE_WHEEL.md").read_text(encoding="utf-8")

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

    def test_autopilot_has_safe_surface_range_arrival_and_deceleration(self) -> None:
        self.assertIn("JoseArrivalSurfaceRangeMeters", self.controller_h)
        self.assertIn("JoseArrivalToleranceMeters", self.controller_h)
        self.assertIn("JoseApproachGainPerSecond", self.controller_h)
        self.assertIn("Target.SurfaceRangeMeters - JoseArrivalSurfaceRangeMeters", self.autopilot_cpp)
        self.assertIn("FMath::Clamp", self.autopilot_cpp)
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
        self.assertIn("CancelJoseTakeTheWheel(false, false)", self.tick_cpp)
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
