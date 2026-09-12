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


class GraphicsScalabilityConfigTests(unittest.TestCase):
    def _variables(self, filename: str) -> dict[str, str]:
        parser = configparser.ConfigParser(strict=True)
        parser.optionxform = str
        parser.read(CONFIG_DIR / filename, encoding="utf-8")
        self.assertIn("ConsoleVariables", parser)
        return dict(parser["ConsoleVariables"])

    def test_low_spec_profile_is_presentation_only(self) -> None:
        variables = self._variables("LowSpecPlay.ini")
        self.assertEqual(variables["t.MaxFPS"], "30")
        self.assertLessEqual(int(variables["r.Streaming.PoolSize"]), 512)
        for key in variables:
            self.assertTrue(
                key.lower().startswith(PRESENTATION_PREFIXES),
                f"LowSpecPlay.ini contains non-presentation key: {key}",
            )

    def test_emergency_profile_is_no_more_expensive_than_low(self) -> None:
        low = self._variables("LowSpecPlay.ini")
        emergency = self._variables("EmergencyMinimum.ini")
        self.assertLessEqual(
            int(emergency["r.ScreenPercentage"]), int(low["r.ScreenPercentage"])
        )
        self.assertLessEqual(
            int(emergency["r.Streaming.PoolSize"]), int(low["r.Streaming.PoolSize"])
        )
        for key in emergency:
            self.assertTrue(
                key.lower().startswith(PRESENTATION_PREFIXES),
                f"EmergencyMinimum.ini contains non-presentation key: {key}",
            )


if __name__ == "__main__":
    unittest.main()
