from pathlib import Path
import unittest


class LowSpecPlaytestLauncherTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        cls.launcher = (repo_root / "tools" / "run_phase2_first_playtest.ps1").read_text(
            encoding="utf-8"
        )

    def test_low_spec_mode_is_explicit_and_opt_in(self) -> None:
        self.assertIn("[switch]$LowSpec", self.launcher)
        self.assertIn("if ($LowSpec)", self.launcher)
        self.assertIn("Low-Spec Development", self.launcher)

    def test_low_spec_mode_bounds_build_and_render_cost(self) -> None:
        for expected in ("-MaxParallelActions=2", "-WINDOWED", "-ResX=1280", "-ResY=720"):
            self.assertIn(expected, self.launcher)

    def test_launcher_reads_profiles_instead_of_duplicating_cvars(self) -> None:
        self.assertIn("function Get-PresentationProfileCommands", self.launcher)
        self.assertIn("LowSpecPlay.ini", self.launcher)
        self.assertIn("EmergencyMinimum.ini", self.launcher)
        for hardcoded in ("r.ScreenPercentage 65", "r.Streaming.PoolSize 384", "sg.ShadowQuality 0"):
            self.assertNotIn(hardcoded, self.launcher)

    def test_profiles_bound_render_and_memory_cost(self) -> None:
        import configparser

        config_dir = Path(__file__).resolve().parents[1] / "unreal" / "Config"
        expected = {
            "LowSpecPlay.ini": ("30", "65", "384"),
            "EmergencyMinimum.ini": ("30", "50", "256"),
        }
        for name, (fps, pct, pool) in expected.items():
            parser = configparser.ConfigParser(strict=True)
            parser.optionxform = str
            parser.read(config_dir / name, encoding="utf-8")
            v = parser["ConsoleVariables"]
            with self.subTest(profile=name):
                self.assertEqual((v["t.MaxFPS"], v["r.ScreenPercentage"], v["r.Streaming.PoolSize"]), (fps, pct, pool))
                for key in ("sg.ShadowQuality", "sg.GlobalIlluminationQuality", "sg.ReflectionQuality", "r.VolumetricFog"):
                    self.assertEqual(v[key], "0")

    def test_default_launch_path_does_not_apply_low_spec_commands_unconditionally(self) -> None:
        low_spec_index = self.launcher.index("if ($LowSpec) {", self.launcher.index("$EditorArguments"))
        exec_cmds_index = self.launcher.index("-ExecCmds=", low_spec_index)
        else_index = self.launcher.index("else {", exec_cmds_index)
        self.assertLess(low_spec_index, exec_cmds_index)
        self.assertLess(exec_cmds_index, else_index)

    def test_emergency_minimum_profile_is_explicit(self) -> None:
        for expected in ("[switch]$EmergencyMinimum", "if ($LowSpec -and $EmergencyMinimum)", "-ResX=960", "-ResY=540"):
            self.assertIn(expected, self.launcher)


if __name__ == "__main__":
    unittest.main()
