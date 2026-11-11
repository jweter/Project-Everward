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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03ArmToggleTest,
    "Everward.G03.Native.ArmToggleReversesAndJointControlsStayHonest",
    EverwardNativeFlags)

bool FEverwardG03ArmToggleTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;
    const EEverwardManipulatorArmId Arm = EEverwardManipulatorArmId::Port;

    auto ArmState = [Adapter, Arm](FEverwardManipulatorArmState& Out) {
        const TArray<FEverwardManipulatorArmState> States =
            Adapter->GetManipulatorArmStates();
        for (const FEverwardManipulatorArmState& State : States)
        {
            if (State.ArmId == Arm)
            {
                Out = State;
                return true;
            }
        }
        return false;
    };

    FEverwardManipulatorArmState State;
    if (!TestTrue(TEXT("Port arm state is available"), ArmState(State))) return false;
    TestFalse(TEXT("Port arm begins stowed"), State.bIsDeployed);
    TestFalse(TEXT("Stowed arm refuses joint controls"), State.bAcceptsJointCommands);

    if (!TestTrue(TEXT("Toggle starts deployment"),
        Adapter->CommandToggleManipulatorArmDeployment(Arm).bAccepted)) return false;
    if (!TestTrue(TEXT("Deploying state is readable"), ArmState(State))) return false;
    TestTrue(TEXT("First toggle points toward deployed"), State.bIsDeploying);
    TestFalse(TEXT("Joint controls disabled while deploying"), State.bAcceptsJointCommands);
    TestFalse(TEXT("Joint command rejected during deployment"),
        Adapter->CommandSetManipulatorJointTargetDegrees(
            Arm, EEverwardManipulatorJoint::Shoulder, 5.0).bAccepted);

    if (!Fixture.TickFor(0.20f, this)) return false;
    if (!TestTrue(TEXT("Toggle reverses mid-deployment to stow"),
        Adapter->CommandToggleManipulatorArmDeployment(Arm).bAccepted)) return false;
    if (!TestTrue(TEXT("Stowing state is readable"), ArmState(State))) return false;
    TestTrue(TEXT("Second toggle reverses deployment"), State.bIsStowing);
    TestFalse(TEXT("Joint controls disabled while stowing"), State.bAcceptsJointCommands);

    if (!Fixture.TickFor(0.10f, this)) return false;
    if (!TestTrue(TEXT("Toggle reverses mid-stow back to deploy"),
        Adapter->CommandToggleManipulatorArmDeployment(Arm).bAccepted)) return false;
    if (!TestTrue(TEXT("Redeploy state is readable"), ArmState(State))) return false;
    TestTrue(TEXT("Third toggle reverses stowing"), State.bIsDeploying);
    // 2.2 s in two bounded calls: a full deploy or stow takes exactly 2.0 s (kDeployStowDurationS), and the fixture's float32
    // 0.02 s step makes 100 frames total 1.99999995 s, which would leave the arm a hair short of its end state.
    if (!Fixture.TickFor(1.5f, this) || !Fixture.TickFor(0.7f, this)) return false;
    if (!TestTrue(TEXT("Final deployed state is readable"), ArmState(State))) return false;
    TestTrue(TEXT("Arm fully deploys after reversal"),
        State.bIsDeployed && !State.bIsDeploying && !State.bIsStowing);
    TestTrue(TEXT("Joint controls enabled only when fully deployed"),
        State.bAcceptsJointCommands);
    TestTrue(TEXT("Fully deployed arm accepts neutral joint command"),
        Adapter->CommandSetManipulatorJointTargetDegrees(
            Arm, EEverwardManipulatorJoint::Shoulder, 0.0).bAccepted);

    if (!TestTrue(TEXT("Resting deployment toggles to stow"),
        Adapter->CommandToggleManipulatorArmDeployment(Arm).bAccepted)) return false;
    if (!TestTrue(TEXT("Final stow transition state readable"), ArmState(State))) return false;
    TestTrue(TEXT("Stowing state is signaled"), State.bIsStowing);
    TestFalse(TEXT("Stow disables joint controls even if deployed flag remains true"),
        State.bAcceptsJointCommands);
    TestFalse(TEXT("Joint command rejected during full-arm stow"),
        Adapter->CommandSetManipulatorJointTargetDegrees(
            Arm, EEverwardManipulatorJoint::Shoulder, 5.0).bAccepted);

    // 2.2 s in two bounded calls: a full deploy or stow takes exactly 2.0 s (kDeployStowDurationS), and the fixture's float32
    // 0.02 s step makes 100 frames total 1.99999995 s, which would leave the arm a hair short of its end state.
    if (!Fixture.TickFor(1.5f, this) || !Fixture.TickFor(0.7f, this)) return false;
    if (!TestTrue(TEXT("Completed stow state readable"), ArmState(State))) return false;
    TestFalse(TEXT("Arm eventually fully stows"), State.bIsDeployed);
    TestFalse(TEXT("No stale stow transition remains"), State.bIsStowing);
    TestFalse(TEXT("Joint commands remain unavailable when stowed"),
        State.bAcceptsJointCommands);
    return !HasAnyErrors();
}

// First-playable-loop step 9: the in-engine mirror of the engine-independent repeatable objective cycle
// (src/simulation/include/everward/simulation/objective_cycle.hpp). Rules stay in simulation; this only checks
// that the adapter forwards to them and mirrors the result. Driving damage then repair to Completed in-engine
// is covered by the portable ctest objective_cycle suite and stays PENDING here until an in-engine damage path exists.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03RestorationObjectiveCycleTest,
    "Everward.G03.Native.RestorationObjectiveCycleMirror",
    EverwardNativeFlags)

bool FEverwardG03RestorationObjectiveCycleTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;

    FEverwardRestorationObjectiveCycleStatus Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Cycle starts Idle"), Cycle.State == EEverwardRestorationObjectiveCycleState::Idle);
    TestEqual(TEXT("No cycle completed yet"), Cycle.CompletedCycles, 0);
    TestFalse(TEXT("No damage observed before a cycle starts"), Cycle.bDamageObservedThisCycle);

    // Advancing while Idle is a no-op: nothing triggers a cycle implicitly.
    if (!Fixture.TickFor(0.20f, this)) return false;
    TestTrue(TEXT("Ticking while Idle leaves the cycle Idle"),
        Adapter->GetRestorationObjectiveCycleStatus().State == EEverwardRestorationObjectiveCycleState::Idle);

    // Save while Idle so a later load must bring the cycle back to Idle (review finding: cycle state is persisted).
    if (!TestTrue(TEXT("Idle probe state can be saved"), Adapter->CommandSaveGame().bAccepted)) return false;

    if (!TestTrue(TEXT("Trigger starts the first cycle"),
        Adapter->CommandTriggerNextRestorationCycle().bAccepted))
    {
        return false;
    }
    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Triggered cycle is Active"), Cycle.State == EEverwardRestorationObjectiveCycleState::Active);
    TestFalse(TEXT("Re-trigger while Active is rejected"),
        Adapter->CommandTriggerNextRestorationCycle().bAccepted);

    // A fully restored probe that never took damage must not trivially complete the cycle.
    TestTrue(TEXT("Fresh probe reports starter restoration satisfied"),
        Adapter->GetStarterRestorationStatus().bSatisfied);
    if (!Fixture.TickFor(0.50f, this)) return false;
    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Undamaged probe leaves the cycle Active"),
        Cycle.State == EEverwardRestorationObjectiveCycleState::Active);
    TestEqual(TEXT("No completion without observed damage"), Cycle.CompletedCycles, 0);
    TestFalse(TEXT("Damage latch stays clear without damage"), Cycle.bDamageObservedThisCycle);

    // Loading the earlier save must replace the later in-session cycle state, not keep it.
    if (!TestTrue(TEXT("Earlier save loads"), Adapter->CommandLoadGame().bAccepted)) return false;
    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Load restores the saved Idle cycle"), Cycle.State == EEverwardRestorationObjectiveCycleState::Idle);
    TestEqual(TEXT("Load restores the saved completed-cycle count"), Cycle.CompletedCycles, 0);
    TestFalse(TEXT("Load restores the saved damage latch"), Cycle.bDamageObservedThisCycle);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
