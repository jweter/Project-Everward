from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"


class UnrealJsonLinkerContractTests(unittest.TestCase):
    def test_production_module_declares_json_when_recorder_uses_json(self) -> None:
        build_cs = (SOURCE / "Everward.Build.cs").read_text(encoding="utf-8")
        recorder_cpp = (SOURCE / "PlaytestRecorderActor.cpp").read_text(encoding="utf-8")

        self.assertIn('Serialization/JsonSerializer.h', recorder_cpp)
        self.assertIn('Serialization/JsonWriter.h', recorder_cpp)
        self.assertIn('"Json"', build_cs)


if __name__ == "__main__":
    unittest.main()
