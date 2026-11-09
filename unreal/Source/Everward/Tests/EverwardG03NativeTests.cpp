// G03: free native Unreal runtime/actor tests, never paid Aura testing.
// Uses real UWorld, AEverwardProbePawn and UProbeSimulationAdapter lifecycle.
// G01/G02 full controller-input / contact / repair evidence remains separate.
#include "EverwardPhase2TestEnvironment.h"
#include "EverwardProbePawn.h"
#include "ProbeSimulationAdapter.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
constexpr EAutomationTestFlags EverwardNativeFlags =
    EAutomationTestFlags::EditorContext |
    EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::CommandletContext |
    EAutomationTestFlags::ProductFilter;

struct FEverwardNativeTestFixture
{
    FTestWorldWrapper World;
    AEverwardProbePawn* Pawn = nullptr;
    UProbeSimulationAdapter* Adapter = nullptr;

    bool Start(FAutomationTestBase* Test)
    {
        if (!World.CreateTestWorld(EWorldType::Game))
        {
            World.ForwardErrorMessages(Test);
            return false;
        }
        UWorld* GameWorld = World.GetTestWorld();
        if (!GameWorld)
        {
            Test->AddError(TEXT("Native G03 game world could not be created"));
            return false;
        }
        Pawn = GameWorld->SpawnActor<AEverwardProbePawn>(
            AEverwardProbePawn::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        if (!Pawn)
        {
            Test->AddError(TEXT("Native G03 probe actor could not be spawned"));
            return false;
        }
        if (!World.BeginPlayInTestWorld())
        {
            World.ForwardErrorMessages(Test);
            return false;
        }
        Adapter = Pawn->GetSimulationAdapter();
        if (!Adapter)
        {
            Test->AddError(TEXT("Native G03 probe has no simulation adapter"));
            return false;
        }
        return true;
    }

    bool TickFor(float Seconds, FAutomationTestBase* Test)
    {
        constexpr float StepSeconds = 0.02f;
        const int32 Frames = FMath::CeilToInt(Seconds / StepSeconds);
        if (Frames > 100 || Frames < 0)
        {
            Test->AddError(TEXT("Native G03 requested an unbounded tick interval"));
            return false;
        }
        for (int32 Frame = 0; Frame < Frames; ++Frame)
        {
            if (!World.TickTestWorld(StepSeconds))
            {
                World.ForwardErrorMessages(Test);
                return false;
            }
        }
        return true;
    }
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03ScanSurveyTest,
    "Everward.G03.Native.ScanSurveyPersistsAfterCancel",
    EverwardNativeFlags)

bool FEverwardG03ScanSurveyTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;
    const FString TargetId = AEverwardPhase2TestEnvironment::BootstrapScanTargetId;

    TestFalse(TEXT("Bootstrap resource starts unsurveyed"), Adapter->GetMiningStatus().bSurveyed);
    if (!TestTrue(TEXT("Sensors can be powered"),
        Adapter->CommandAllocatePower(EEverwardPowerSubsystem::Sensors, 100.0).bAccepted))
    {
        return false;
    }
    if (!TestTrue(TEXT("Live Unreal actor accepts scan"),
        Adapter->CommandStartScan(TargetId, 0.25).bAccepted))
    {
        return false;
    }
    if (!Fixture.TickFor(0.80f, this)) return false;
    TestTrue(TEXT("Native domain event confirms scan completion"),
        Adapter->GetLastScanLifecycleNotice().bCompleted);
    TestTrue(TEXT("Completed scan latches actual survey"),
        Adapter->GetMiningStatus().bSurveyed);
    TestTrue(TEXT("Game world advanced authoritative fixed-step time"),
        Adapter->GetSimulationTick() > 0);

    // A second scan gets cancelled, but never erases the first survey.
    if (!TestTrue(TEXT("Second scan starts"),
        Adapter->CommandStartScan(TargetId, 5.0).bAccepted))
    {
        return false;
    }
    if (!TestTrue(TEXT("Second scan cancels"),
        Adapter->CommandCancelScan().bAccepted))
    {
        return false;
    }
    if (!Fixture.TickFor(0.10f, this)) return false;
    TestTrue(TEXT("Scan cancellation event is real"),
        Adapter->GetLastScanLifecycleNotice().bCancelled);
    TestTrue(TEXT("Earlier completed survey survives cancellation"),
        Adapter->GetMiningStatus().bSurveyed);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03RejectedMiningTest,
    "Everward.G03.Native.MiningRejectsWithoutSurveyAndTool",
    EverwardNativeFlags)

bool FEverwardG03RejectedMiningTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;
    const FEverwardMiningStatus Before = Adapter->GetMiningStatus();
    const FEverwardProbeTelemetry TelemetryBefore = Adapter->GetProbeTelemetry();

    const FEverwardProbeCommandResult Attempt = Adapter->CommandMineBootstrapTarget();
    TestFalse(TEXT("Unsurveyed/unequipped mining is rejected"), Attempt.bAccepted);

    const FEverwardMiningStatus After = Adapter->GetMiningStatus();
    const FEverwardProbeTelemetry TelemetryAfter = Adapter->GetProbeTelemetry();
    TestEqual(TEXT("Rejected attempt cannot credit mined mass"),
        After.ExtractedMaterialKilograms, Before.ExtractedMaterialKilograms);
    TestEqual(TEXT("Rejected attempt cannot deplete resource deposit"),
        After.DepositRemainingKilograms, Before.DepositRemainingKilograms);
    TestEqual(TEXT("Rejected attempt cannot credit storage"),
        TelemetryAfter.StorageUsedKilograms, TelemetryBefore.StorageUsedKilograms);
    TestTrue(TEXT("No fabricated material inventory"),
        Adapter->GetStoredMaterialInventory().IsEmpty());
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
