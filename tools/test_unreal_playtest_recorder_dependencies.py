from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal/Source/Everward"


class UnrealPlaytestRecorderDependencyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.build_cs = (SOURCE / "Everward.Build.cs").read_text(encoding="utf-8")
        self.recorder_cpp = (SOURCE / "PlaytestRecorderActor.cpp").read_text(encoding="utf-8")

    def test_json_using_recorder_links_json_module(self) -> None:
        self.assertIn('Serialization/JsonSerializer.h', self.recorder_cpp)
        self.assertIn('Serialization/JsonWriter.h', self.recorder_cpp)
        self.assertIn('"Json"', self.build_cs)


if __name__ == "__main__":
    unittest.main()
