import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class Phase2PlaytestCaptureAndMiningControlsTest(unittest.TestCase):
    def test_production_game_spawns_non_blocking_recorder(self):
        game_mode = (ROOT / "unreal/Source/Everward/EverwardGameMode.cpp").read_text(encoding="utf-8")
        recorder = (ROOT / "unreal/Source/Everward/PlaytestRecorderActor.cpp").read_text(encoding="utf-8")
        self.assertIn("APlaytestRecorderActor", game_mode)
        self.assertIn("continuing without structured capture", game_mode)
        self.assertIn("SavedDir", recorder)
        self.assertIn("Playtests", recorder)
        self.assertIn("events.jsonl", recorder)
        self.assertIn("telemetry.csv", recorder)
        self.assertIn("EKeys::F12", recorder)
        self.assertNotIn("EKeys::F9", recorder)

    def test_mining_reach_uses_visible_tool_tip(self):
        mining = (ROOT / "src/simulation/include/everward/simulation/mining.hpp").read_text(encoding="utf-8")
        tool_contact = (ROOT / "src/simulation/include/everward/simulation/manipulator_tool_contact.hpp").read_text(encoding="utf-8")
        self.assertIn("manipulator_tool_tip_contact_sample", mining)
        self.assertIn("tool-tip surface gap", mining)
        self.assertIn("kTipOffsetFromWristM", tool_contact)
        self.assertIn("angles.wrist_degrees", tool_contact)

    def test_player_has_selected_arm_and_target_navigation_controls(self):
        tick = (ROOT / "unreal/Source/Everward/EverwardPlayerControllerInteractionTick.cpp").read_text(encoding="utf-8")
        controls = (ROOT / "unreal/Source/Everward/EverwardPlayerControllerMiningControls.cpp").read_text(encoding="utf-8")
        for key in ("EKeys::Seven", "EKeys::H", "EKeys::P", "EKeys::G"):
            self.assertIn(key, tick)
        self.assertIn("GetSelectedManipulatorArmIndex", controls)
        self.assertIn("ToggleSelectedManipulatorTool", controls)
        self.assertIn("CycleMiningTarget", controls)
        self.assertIn("ToggleAutoApproachMiningTarget", controls)
        self.assertIn("StagingPointMeters", controls)
        self.assertIn("bSurveyed", controls)

    def test_playtest_documentation_matches_production_controls(self):
        doc = (ROOT / "docs/PLAYTESTING.md").read_text(encoding="utf-8")
        self.assertIn("unreal/Saved/Playtests", doc)
        self.assertIn("F12", doc)
        self.assertIn("F9 conflicts", doc)
        self.assertIn("N     cycle selected manipulator arm", doc)
        self.assertIn("P     engage/cancel auto-approach", doc)


if __name__ == "__main__":
    unittest.main()
