// Contextual action guidance is a read-only, no-world HUD projection.
// The adapter and simulation remain authoritative for action acceptance.
#include "ProbeHUDReadModel.h"
#include "ProbeSimulationAdapter.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03ContextualHUDActionHintTest,
    "Everward.G03.Native.ContextualHUDActionHint",
    EAutomationTestFlags::EditorContext |
    EAutomationTestFlags::CommandletContext |
    EAutomationTestFlags::ProductFilter)

bool FEverwardG03ContextualHUDActionHintTest::RunTest(const FString& Parameters)
{
    FEverwardProbeTelemetry Telemetry;
    Telemetry.ProbeId = TEXT("EV-0001");
    Telemetry.StorageCapacityKilograms = 20.0;
    FEverwardTargetSelectionStatus Target;
    FEverwardTargetKnowledgeStatus Knowledge;
    FEverwardMiningStatus Mining;
    Mining.TargetId = TEXT("phase2-test-target-001");
    Mining.DepositRemainingKilograms = 50.0;
    Mining.ToolWorkingReachMeters = 0.55;
    FEverwardProbeCapability Sensors;
    Sensors.CapabilityId = FName(TEXT("sensors"));
    Sensors.bAvailable = true;
    Sensors.bOperational = true;
    TArray<FEverwardProbeCapability> Capabilities;
    Capabilities.Add(Sensors);
    TArray<FEverwardManipulatorArmState> Arms;

    const auto Build = [&]()
    {
        return FEverwardHUDActionHint::FromAuthoritativeState(
            Telemetry, Target, Knowledge, Mining, Capabilities, Arms);
    };

    TestTrue(TEXT("No selection suggests the real T key"), Build().Text.Contains(TEXT("[T]")));
    TestFalse(TEXT("No selection is not a warning"), Build().bWarning);

    Target.bHasSelection = true;
    Target.TargetId = Mining.TargetId;
    TestTrue(TEXT("Unsurveyed bootstrap target suggests the real TAB/sensors/Enter scan path"),
        Build().Text.Contains(TEXT("[TAB]")) && Build().Text.Contains(TEXT("[ENTER]")));
    Capabilities[0].bAvailable = false;
    TestTrue(TEXT("Unavailable sensors warn instead of suggesting scanning"),
        Build().bWarning && Build().Text.Contains(TEXT("SENSORS BLOCKED")));
    Capabilities[0].bAvailable = true;

    Telemetry.bIsScanning = true;
    Telemetry.ActiveScanTargetId = Mining.TargetId;
    Telemetry.ScanRemainingSeconds = 2.5;
    TestTrue(TEXT("Active scan shows the bound target, not just the countdown"),
        Build().Text.Contains(TEXT("...001")) && Build().Text.Contains(TEXT("2.5 S LEFT")));
    Target.TargetId = TEXT("phase2-test-target-003");
    TestTrue(TEXT("Changing selected target cannot re-label the active scan"),
        Build().Text.Contains(TEXT("...001")) && !Build().Text.Contains(TEXT("...003")));
    Capabilities[0].bAvailable = false;
    TestTrue(TEXT("Disabled sensors pause the active scan hint"),
        Build().bWarning && Build().Text.Contains(TEXT("PAUSED")));
    Capabilities[0].bAvailable = true;
    Telemetry.bIsScanning = false;
    Target.TargetId = Mining.TargetId;

    Mining.bSurveyed = true;
    TestTrue(TEXT("Without a deployed tool, hint shows actual arm controls"),
        Build().Text.Contains(TEXT("[1/2]")) && Build().Text.Contains(TEXT("[7]")));
    FEverwardManipulatorArmState Arm;
    Arm.bIsDeployed = true;
    Arm.bToolAttached = true;
    Arms.Add(Arm);
    Target.SurfaceRangeMeters = 50.0;
    TestTrue(TEXT("Surveyed deposit with tool suggests attempts, not guaranteed mining"),
        Build().Text.Contains(TEXT("[P] TRY")) && Build().Text.Contains(TEXT("[G] TRY")));

    Telemetry.StorageUsedKilograms = 20.0;
    TestTrue(TEXT("Full authoritative storage blocks mining hint"),
        Build().bWarning && Build().Text.Contains(TEXT("STORAGE FULL")));
    Telemetry.StorageUsedKilograms = 0.0;
    Mining.DepositRemainingKilograms = 0.0;
    TestTrue(TEXT("Exhausted deposit is not offered for mining"),
        Build().Text.Contains(TEXT("EXHAUSTED")));
    Mining.DepositRemainingKilograms = 50.0;

    Target.TargetId = TEXT("phase2-test-target-004");
    TestFalse(TEXT("Non-bootstrap target never inherits mining readiness"),
        Build().Text.Contains(TEXT("[G]")));
    TestTrue(TEXT("Unobserved reference target now suggests an actual selected-target scan"),
        Build().Text.Contains(TEXT("[ENTER] SCAN")));
    Knowledge.bHasKnowledge = true;
    Knowledge.Level = EEverwardKnowledgeLevel::Observed;
    Knowledge.Confidence = 1.0;
    TestTrue(TEXT("Fully observed target is not trapped in a rescan loop"),
        Build().Text.Contains(TEXT("OBSERVED")) && Build().Text.Contains(TEXT("[F] TRY GRASP")));
    Knowledge.Level = EEverwardKnowledgeLevel::Characterized;
    TestTrue(TEXT("Real characterized knowledge can be surfaced without mining claims"),
        Build().Text.Contains(TEXT("CHARACTERIZED")) && !Build().Text.Contains(TEXT("[G]")));

    Telemetry.bIsOverheated = true;
    TestTrue(TEXT("Thermal lockout warns before normal actions"),
        Build().bWarning && Build().Text.Contains(TEXT("THERMAL LOCKOUT")));
    Telemetry.bIsOverheated = false;
    Telemetry.bIsEnergyDepleted = true;
    TestTrue(TEXT("Energy depletion warns before normal actions"),
        Build().bWarning && Build().Text.Contains(TEXT("ENERGY DEPLETED")));

    return true;
}
#endif
