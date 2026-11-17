// Issue #384: native, no-world contract for the compact HUD presentation
// projection. Data arrives from the established authoritative adapter and
// Fix_It actor; this test deliberately never changes gameplay state.
#include "ProbeHUDReadModel.h"
#include "ProbeSimulationAdapter.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03CompactHUDReadModelTest,
    "Everward.G03.Native.CompactHUDReadModel",
    EAutomationTestFlags::EditorContext |
    EAutomationTestFlags::CommandletContext |
    EAutomationTestFlags::ProductFilter)

bool FEverwardG03CompactHUDReadModelTest::RunTest(const FString& Parameters)
{
    FEverwardProbeTelemetry Telemetry;
    Telemetry.ProbeId = TEXT("EV-0001");
    Telemetry.Generation = 1;
    Telemetry.StoredEnergyJoules = 25.0;
    Telemetry.EnergyCapacityJoules = 100.0;
    Telemetry.StorageUsedKilograms = 4.0;
    Telemetry.StorageCapacityKilograms = 8.0;
    Telemetry.SensorsIntegrity = 0.8;
    Telemetry.PropulsionIntegrity = 0.4;
    Telemetry.ComputationIntegrity = 0.18;
    Telemetry.ThermalIntegrity = 0.22;
    Telemetry.TemperatureKelvin = 290.0;
    Telemetry.bIsScanning = true;

    FEverwardTargetSelectionStatus Target;
    Target.bHasSelection = true;
    Target.TargetId = TEXT("rock-01");
    Target.SurfaceRangeMeters = 3.5;

    const FEverwardHUDReadModel Status = FEverwardHUDReadModel::FromAuthoritativeState(
        Telemetry, Target, TEXT("FIX_IT // REPAIRING COMPUTATION"));
    TestTrue(TEXT("Probe is initialized"), Status.bHasProbeState);
    TestTrue(TEXT("Energy normalized against capacity"),
        FMath::IsNearlyEqual(Status.EnergyFraction, 0.25f));
    TestTrue(TEXT("Weakest actual subsystem is computation, not invented hull integrity"),
        FMath::IsNearlyEqual(Status.WeakestSubsystemFraction, 0.18f));
    TestTrue(TEXT("Storage normalized against capacity"),
        FMath::IsNearlyEqual(Status.StorageFraction, 0.5f));
    TestTrue(TEXT("Temperature remains Kelvin"), FMath::IsNearlyEqual(Status.CoreTemperatureKelvin, 290.0));
    TestTrue(TEXT("Target uses selected authoritative surface range"),
        Status.bHasTarget && Status.TargetId == TEXT("rock-01") &&
        FMath::IsNearlyEqual(Status.TargetSurfaceRangeMeters, 3.5));
    TestEqual(TEXT("Scanning is currently observable"), Status.CurrentAction, FString(TEXT("SCANNING")));
    TestEqual(TEXT("Fix_It summary is passed through without simulation"),
        Status.FixItStatus, FString(TEXT("FIX_IT // REPAIRING COMPUTATION")));

    Telemetry.StoredEnergyJoules = 200.0;
    Telemetry.EnergyCapacityJoules = 0.0;
    Telemetry.StorageUsedKilograms = -10.0;
    Telemetry.StorageCapacityKilograms = 10.0;
    Target.bHasSelection = false;
    const FEverwardHUDReadModel Invalid = FEverwardHUDReadModel::FromAuthoritativeState(
        Telemetry, Target, FString());
    TestTrue(TEXT("Invalid capacity produces no fabricated percentage"),
        FMath::IsNearlyZero(Invalid.EnergyFraction));
    TestTrue(TEXT("Negative storage clamps to zero"),
        FMath::IsNearlyZero(Invalid.StorageFraction));
    TestFalse(TEXT("Unselected target has no misleading range"), Invalid.bHasTarget);
    TestEqual(TEXT("Unavailable Fix_It is explicit"),
        Invalid.FixItStatus, FString(TEXT("FIX_IT // UNAVAILABLE")));
    return true;
}
#endif
