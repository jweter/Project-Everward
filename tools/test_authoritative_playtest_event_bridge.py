"""Source-contract guards for the Unreal playtest recorder bridge.

These checks run without Unreal and DO NOT constitute real-game acceptance.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
ADAPTER = (ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
ADAPTER_HEADER = (ROOT / "unreal/Source/Everward/ProbeSimulationAdapter.h").read_text(encoding="utf-8")
MINING = (ROOT / "unreal/Source/Everward/ProbeMiningBridge.cpp").read_text(encoding="utf-8")
RECORDER = (ROOT / "unreal/Source/Everward/PlaytestRecorderActor.cpp").read_text(encoding="utf-8")


class AuthoritativePlaytestEventBridgeTests(unittest.TestCase):
    def test_scan_completion_recorded_from_real_domain_events(self):
        source = ADAPTER.split("const auto Events = Core->drain_events();", 1)[1].split("if (Manipulators != nullptr)", 1)[0]
        self.assertIn("DomainEventType::ScanStarted", source)
        self.assertIn('TEXT("scan_started")', source)
        self.assertIn("DomainEventType::ScanCompleted", source)
        self.assertIn('TEXT("scan_completed")', source)
        self.assertIn('TEXT("scan_cancelled")', source)
        self.assertIn("LastScanLifecycleNotice.bCompleted ? ", source)
        self.assertIn("RecordSimulationPlaytestEvent(", source)
        self.assertNotIn('RecordSimulationPlaytestEvent(TEXT("scan_completed")', ADAPTER.split(
            "FEverwardProbeCommandResult UProbeSimulationAdapter::CommandStartScan(", 1)[1].split(
            "FEverwardProbeCommandResult UProbeSimulationAdapter::CommandCancelScan", 1)[0])

    def test_mining_success_event_only_after_accepted_extraction_and_real_storage_delta(self):
        accepted = MINING.split("if (Result.accepted)", 1)[1].split("LastAutomationNotice.Sequence", 1)[0]
        self.assertIn("Core->add_stored_material_kg(Result.extracted_kg, Result.material_id)", accepted)
        self.assertIn("StorageBeforeKilograms = Snapshot.storage_used_kg", MINING)
        self.assertIn("StorageAfterKilograms = Core->snapshot().storage_used_kg", accepted)
        self.assertIn("Result.extracted_kg > 0.0 && StorageAfterKilograms > StorageBeforeKilograms", accepted)
        delta_gate = accepted.split("if (Result.extracted_kg > 0.0 && StorageAfterKilograms > StorageBeforeKilograms)", 1)[1]
        self.assertIn('TEXT("material_collected")', delta_gate)
        self.assertIn('TEXT("mining_reach_verified")', delta_gate)
        self.assertIn("Result.tool_surface_gap_m", delta_gate)
        self.assertIn("Core->tick()", delta_gate)
        self.assertNotIn('TEXT("mining_contact")', MINING)
        self.assertLess(delta_gate.index('TEXT("mining_reach_verified")'),
                        delta_gate.index('TEXT("material_collected")'))

    def test_rejected_mining_records_diagnostic_never_false_extraction(self):
        reject = MINING.split("if (!Result.accepted)", 1)[1].split("LastAutomationNotice.Sequence", 1)[0]
        self.assertIn('TEXT("mining_attempt_rejected")', reject)
        self.assertIn("Result.detail.c_str()", reject)
        self.assertNotIn('TEXT("material_collected")', reject)
        self.assertNotIn('TEXT("mining_reach_verified")', reject)

    def test_bridge_optional_nonblocking_and_does_not_change_simulation_state(self):
        self.assertIn("void RecordSimulationPlaytestEvent(const FString& EventName, const FString& Detail) const;", ADAPTER_HEADER)
        method = ADAPTER.split("void UProbeSimulationAdapter::RecordSimulationPlaytestEvent(", 1)[1].split(
            "void UProbeSimulationAdapter::SyncOwnerTransformFromSimulation()", 1)[0]
        self.assertIn("if (World == nullptr)", method)
        self.assertIn("TActorIterator<APlaytestRecorderActor>", method)
        self.assertIn("It->RecordPlaytestEvent(EventName, Detail)", method)
        self.assertNotIn("Core->", method)
        self.assertNotIn("SpawnActor", method)
        self.assertNotIn("RequestExit", method)
        self.assertIn('const FString Header = TEXT("timestamp_utc', RECORDER)

    def test_reach_is_not_misrepresented_as_actual_visual_contact(self):
        self.assertIn("MiningSystem::kGeneration1ToolWorkingReachM", MINING)
        self.assertIn('"Reach" means within the Gen-1', MINING)
        self.assertNotIn('TEXT("mining_contact")', MINING)


if __name__ == "__main__":
    unittest.main()
