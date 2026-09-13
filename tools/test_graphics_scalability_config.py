import configparser
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
CONFIG_DIR = ROOT / "unreal" / "Config"

PRESENTATION_PREFIXES = (
    "r.",
    "sg.",
    "t.maxfps",
)

PRESET_FILES = (
    "EmergencyMinimum.ini",
    "LowSpecPlay.ini",
    "MediumPlay.ini",
    "HighPlay.ini",
    "UltraCinematic.ini",
)

MONOTONIC_COST_KEYS = (
    "r.ScreenPercentage",
    "r.Streaming.PoolSize",
    "sg.ViewDistanceQuality",
    "sg.AntiAliasingQuality",
    "sg.ShadowQuality",
    "sg.GlobalIlluminationQuality",
    "sg.ReflectionQuality",
    "sg.PostProcessQuality",
    "sg.TextureQuality",
    "sg.EffectsQuality",
    "sg.FoliageQuality",
    "sg.ShadingQuality",
)


class GraphicsScalabilityConfigTests(unittest.TestCase):
    def _variables(self, filename: str) -> dict[str, str]:
        parser = configparser.ConfigParser(strict=True)
        parser.optionxform = str
        parser.read(CONFIG_DIR / filename, encoding="utf-8")
        self.assertIn("ConsoleVariables", parser)
        return dict(parser["ConsoleVariables"])

    def test_every_preset_is_presentation_only(self) -> None:
        for filename in PRESET_FILES:
            with self.subTest(filename=filename):
                variables = self._variables(filename)
                for key in variables:
                    self.assertTrue(
                        key.lower().startswith(PRESENTATION_PREFIXES),
                        f"{filename} contains non-presentation key: {key}",
                    )

    def test_low_spec_profile_stays_bounded_for_integrated_graphics(self) -> None:
        variables = self._variables("LowSpecPlay.ini")
        self.assertEqual(variables["t.MaxFPS"], "30")
        self.assertLessEqual(int(variables["r.Streaming.PoolSize"]), 512)

    def test_quality_ladder_never_gets_cheaper_as_quality_increases(self) -> None:
        presets = [self._variables(filename) for filename in PRESET_FILES]
        for key in MONOTONIC_COST_KEYS:
            values = [int(preset[key]) for preset in presets]
            with self.subTest(key=key):
                self.assertEqual(values, sorted(values), f"non-monotonic preset cost for {key}: {values}")

    def test_emergency_profile_is_no_more_expensive_than_low(self) -> None:
        low = self._variables("LowSpecPlay.ini")
        emergency = self._variables("EmergencyMinimum.ini")
        self.assertLessEqual(
            int(emergency["r.ScreenPercentage"]), int(low["r.ScreenPercentage"])
        )
        self.assertLessEqual(
            int(emergency["r.Streaming.PoolSize"]), int(low["r.Streaming.PoolSize"])
        )


if __name__ == "__main__":
    unittest.main()
