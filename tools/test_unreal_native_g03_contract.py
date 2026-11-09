"""Static registration/runner safeguards. NOT native Unreal execution proof."""
from __future__ import annotations

import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "unreal" / "Source" / "Everward" / "Tests" / "EverwardG03NativeTests.cpp"
RUNNER = ROOT / "tools" / "run_unreal_native_g03.ps1"


class G03NativeContractTests(unittest.TestCase):
    def test_two_native_unreal_automation_tests_are_registered(self) -> None:
        source = SOURCE.read_text(encoding="utf-8")
        self.assertEqual(source.count("IMPLEMENT_SIMPLE_AUTOMATION_TEST("), 2)
        for name in (
            "Everward.G03.Native.ScanSurveyPersistsAfterCancel",
            "Everward.G03.Native.MiningRejectsWithoutSurveyAndTool",
        ):
            self.assertIn(name, source)
        self.assertIn("WITH_DEV_AUTOMATION_TESTS", source)
        self.assertIn("FTestWorldWrapper", source)
        self.assertIn("EWorldType::Game", source)
        self.assertIn("AEverwardProbePawn", source)
        self.assertIn("GetSimulationAdapter()", source)

    def test_actual_adapter_commands_and_authoritative_state(self) -> None:
        source = SOURCE.read_text(encoding="utf-8")
        for symbol in (
            "CommandStartScan(", "CommandCancelScan(", "CommandMineBootstrapTarget(",
            "GetMiningStatus()", "GetStoredMaterialInventory()",
            "GetProbeTelemetry()", "GetSimulationTick()", "TickTestWorld(",
        ):
            self.assertIn(symbol, source)
        self.assertIn("After.ExtractedMaterialKilograms", source)
        self.assertIn("After.DepositRemainingKilograms", source)
        self.assertIn("TelemetryAfter.StorageUsedKilograms", source)
        self.assertNotIn("Core->", source)
        self.assertNotIn("MiningSystem::mine_once", source)

    def test_runner_requires_a_real_unreal_report(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        for required in (
            "UnrealEditor-Cmd.exe", "Everward.G03.Native", "index.json",
            "ConvertFrom-Json", "failed", "succeeded", "status --porcelain",
            "Get-Process", "FreePhysicalMemory", "ReportExportPath", "TestExit",
        ):
            self.assertIn(required, source, required)
        self.assertIn("no PASS can be inferred", source)
        self.assertNotIn("Start-Process -FilePath $EditorCmd -ArgumentList $Arguments -Wait", source)

    def test_runner_is_no_aura_and_owner_resource_safe(self) -> None:
        source = RUNNER.read_text(encoding="utf-8")
        self.assertNotIn("Autopilot.exe", source)
        self.assertNotIn("aura run", source.lower())
        self.assertIn("MinimumFreeMemoryGiB", source)
        self.assertIn("UnrealBuildTool", source)
        self.assertIn("taskkill.exe /PID $Running.Id", source)
        self.assertIn("Wait-Guarded $BuildProc", source)
        self.assertIn("Wait-Guarded $Process", source)
        self.assertIn("Build.bat", source)
        self.assertIn("exact-HEAD", source)


if __name__ == "__main__":
    unittest.main()
