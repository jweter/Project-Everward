from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal" / "Source" / "Everward"


class ZeroGEnvironmentSurfaceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.header = (SOURCE / "EverwardZeroGTestEnvironment.h").read_text(encoding="utf-8")
        self.cpp = (SOURCE / "EverwardZeroGTestEnvironment.cpp").read_text(encoding="utf-8")
        self.game_mode = (SOURCE / "EverwardGameMode.cpp").read_text(encoding="utf-8")

    def test_dedicated_environment_is_separate_actor(self) -> None:
        self.assertIn("AEverwardZeroGTestEnvironment", self.header)
        self.assertIn("AEverwardPhase2TestEnvironment", self.header)

    def test_zero_g_scene_has_distant_light_and_reference_body(self) -> None:
        self.assertIn("UDirectionalLightComponent", self.cpp)
        self.assertIn("ZeroGAsteroidReference", self.cpp)
        self.assertIn("ECollisionEnabled::NoCollision", self.cpp)

    def test_game_mode_selects_zero_g_by_explicit_option(self) -> None:
        self.assertIn('GetIntOption(Options, TEXT("ZeroG"), 0)', self.game_mode)
        self.assertIn("AEverwardZeroGTestEnvironment::StaticClass()", self.game_mode)

    def test_existing_authoritative_environment_remains_default(self) -> None:
        self.assertIn("AEverwardPhase2TestEnvironment::StaticClass()", self.game_mode)


if __name__ == "__main__":
    unittest.main()
