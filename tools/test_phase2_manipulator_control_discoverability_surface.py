from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorControlDiscoverabilityTests(unittest.TestCase):
    """Keep the Phase-2 manipulator control surface visible to a player who
    has not read source code, commit history, or a separate test document."""

    def setUp(self) -> None:
        self.hud_h = (SOURCE / "EverwardHUD.h").read_text(encoding="utf-8")
        self.hud_cpp = (SOURCE / "EverwardHUD.cpp").read_text(encoding="utf-8")

    def test_manipulator_panel_is_visible_on_first_launch(self) -> None:
        self.assertIn("bool bManipulatorPanelExpanded = true;", self.hud_h)

    def test_open_panel_teaches_complete_current_control_surface(self) -> None:
        for text in (
            "MANIPULATOR CONTROL   [M CLOSE]",
            "[1] PORT ARM // DEPLOY / STOW",
            "[2] STARBOARD ARM // DEPLOY / STOW",
            "[3] TOOL // ATTACH / DETACH",
            "[N] SELECT ARM",
            "[4/5/6] SELECT JOINT",
            "[,] / [.] ADJUST TARGET",
            "[G] MINE SURVEYED TARGET",
            "[F1] ALL CONTROLS",
        ):
            self.assertIn(text, self.hud_cpp)


if __name__ == "__main__":
    unittest.main()
