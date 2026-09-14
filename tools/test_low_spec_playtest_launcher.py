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
        for expected in (
            "-MaxParallelActions=2",
            "-WINDOWED",
            "-ResX=1280",
            "-ResY=720",
            "t.MaxFPS 30",
            "r.ScreenPercentage 65",
            "sg.ShadowQuality 0",
            "sg.GlobalIlluminationQuality 0",
            "r.Streaming.PoolSize 384",
        ):
            self.assertIn(expected, self.launcher)

    def test_default_launch_path_does_not_apply_low_spec_commands_unconditionally(self) -> None:
        low_spec_index = self.launcher.index("if ($LowSpec) {", self.launcher.index("$EditorArguments"))
        exec_cmds_index = self.launcher.index("-ExecCmds=", low_spec_index)
        else_index = self.launcher.index("else {", exec_cmds_index)
        self.assertLess(low_spec_index, exec_cmds_index)
        self.assertLess(exec_cmds_index, else_index)


if __name__ == "__main__":
    unittest.main()
