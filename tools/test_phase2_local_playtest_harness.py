from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
PREPARE_PATH = ROOT / "tools/prepare_phase2_first_run_observation.py"
HARNESS_PATH = ROOT / "tools/run_phase2_first_playtest.ps1"
VALIDATOR_PATH = ROOT / "tools/validate_phase2_first_run_observation.py"


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Phase2LocalPlaytestHarnessTests(unittest.TestCase):
    def test_prepared_observation_is_validator_compatible(self) -> None:
        prepare = load_module(PREPARE_PATH, "phase2_prepare_observation")
        validator = load_module(VALIDATOR_PATH, "phase2_validate_observation")

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "observation.json"
            data = prepare.prepare_observation(
                output,
                git_commit="0123456789abcdef",
                cpu="Test CPU",
                gpu="Test GPU",
            )

            self.assertTrue(output.exists())
            saved = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(saved["git_commit"], "0123456789abcdef")
            self.assertEqual(saved["unreal_engine_version"], "5.8")
            self.assertEqual(saved["host"]["cpu"], "Test CPU")
            self.assertEqual(saved["host"]["gpu"], "Test GPU")
            self.assertNotIn("YYYY-MM-DD", saved["captured_at_utc"])
            self.assertEqual(data, saved)
            self.assertEqual(validator.validate_observation(saved), [])

    def test_windows_harness_uses_exact_project_and_editor_build_target(self) -> None:
        source = HARNESS_PATH.read_text(encoding="utf-8")
        self.assertIn('"unreal\\Everward.uproject"', source)
        self.assertIn("EverwardEditor Win64 Development", source)
        self.assertIn("Engine\\Build\\BatchFiles\\Build.bat", source)
        self.assertIn("Engine\\Binaries\\Win64\\UnrealEditor.exe", source)
        self.assertIn("-WaitMutex", source)
        self.assertIn("-NoHotReloadFromIDE", source)
        self.assertIn('"-log"', source)

    def test_windows_harness_discovers_unreal_58_and_records_build_result(self) -> None:
        source = HARNESS_PATH.read_text(encoding="utf-8")
        self.assertIn("UE58_ROOT", source)
        self.assertIn("Unreal Engine\\5.8", source)
        self.assertIn("C:\\Program Files\\Epic Games\\UE_5.8", source)
        self.assertIn('CheckId "unreal_cpp_build" -Status "pass"', source)
        self.assertIn('CheckId "unreal_cpp_build" -Status "fail"', source)
        self.assertIn("Add-ObservationBlocker", source)
        self.assertIn("prepare_phase2_first_run_observation.py", source)


if __name__ == "__main__":
    unittest.main()
