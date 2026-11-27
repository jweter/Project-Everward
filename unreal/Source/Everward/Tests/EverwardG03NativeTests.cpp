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
    TestEqual(TEXT("Completed scan event reports the target without relying on DrawHUD"),
        Adapter->GetLastScanLifecycleNotice().TargetId, TargetId);
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
    TestEqual(TEXT("Completed scan identity survives a subsequent cancellation"),
        Adapter->GetLastCompletedScanLifecycleNotice().TargetId, TargetId);
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
// G01/G02 first-playable-loop success path: FEverwardG03RejectedMiningTest above
// deliberately proves only the rejection half (no survey, no tool); this drives the
// same adapter commands AEverwardPlayerController's P/G mining controls use
// (ToggleAutoApproachMiningTarget / AlignSelectedArmToMiningPose / CommandMineBootstrapTarget)
// all the way to an accepted mine_once with a real inventory/storage/deposit delta and
// visible feedback. Teleport-free: velocity + ticking moves the probe to the staging
// solution's position the same way live auto-approach does, just without a human waiting.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03MiningSuccessTest,
    "Everward.G03.Native.MiningSuccessCreditsInventoryAndReducesDeposit",
    EverwardNativeFlags)

bool FEverwardG03MiningSuccessTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;
    const FString TargetId = AEverwardPhase2TestEnvironment::BootstrapScanTargetId;
    const EEverwardManipulatorArmId Arm = EEverwardManipulatorArmId::Port;

    // 1. Scan and survey, exactly as ScanSurveyPersistsAfterCancel proves.
    if (!TestTrue(TEXT("Sensors can be powered"),
        Adapter->CommandAllocatePower(EEverwardPowerSubsystem::Sensors, 100.0).bAccepted)) return false;
    if (!TestTrue(TEXT("Scan accepted"), Adapter->CommandStartScan(TargetId, 0.25).bAccepted)) return false;
    if (!Fixture.TickFor(0.80f, this)) return false;
    if (!TestTrue(TEXT("Bootstrap deposit is surveyed"), Adapter->GetMiningStatus().bSurveyed)) return false;

    // 2. Deploy the port arm and attach its tool. 2.2 s split in two calls, as the
    // arm-toggle test above documents: a float32 0.02 s step makes 100 frames fall
    // just short of kDeployStowDurationS's exact 2.0 s.
    if (!TestTrue(TEXT("Port arm begins deploying"),
        Adapter->CommandDeployManipulatorArm(Arm).bAccepted)) return false;
    if (!Fixture.TickFor(1.5f, this) || !Fixture.TickFor(0.7f, this)) return false;

    auto ArmState = [Adapter, Arm](FEverwardManipulatorArmState& Out) {
        for (const FEverwardManipulatorArmState& ArmLoopState : Adapter->GetManipulatorArmStates())
        {
            if (ArmLoopState.ArmId == Arm) { Out = ArmLoopState; return true; }
        }
        return false;
    };
    FEverwardManipulatorArmState State;
    if (!TestTrue(TEXT("Port arm state readable"), ArmState(State))) return false;
    if (!TestTrue(TEXT("Port arm fully deployed before attaching its tool"),
        State.bIsDeployed && !State.bIsDeploying && !State.bIsStowing)) return false;
    if (!TestTrue(TEXT("Tool attaches to the deployed arm"),
        Adapter->CommandAttachManipulatorTool(Arm).bAccepted)) return false;
    if (!TestTrue(TEXT("Attached arm state readable"), ArmState(State))) return false;
    TestTrue(TEXT("Arm reports its tool attached"), State.bToolAttached);

    // 3. The same real kinematic solver #375 and live auto-approach (P) use: a staging
    // position plus the joint angles that put the tool tip in reach.
    const FEverwardMiningStagingSolution Solution = Adapter->GetMiningStagingSolution(Arm, TargetId);
    if (!TestTrue(TEXT("A collision-free mining pose exists for the port arm"), Solution.bHasSolution))
    {
        return false;
    }

    // 4. Move to the staging position the same way auto-approach does (set velocity,
    // let ticking integrate it) rather than teleporting: one second at a velocity
    // numerically equal to the remaining delta covers exactly that delta, then velocity
    // is zeroed before any further ticking so later joint-settle ticks cannot drift the
    // probe back out of reach.
    const FVector StartPositionMeters = Adapter->GetProbePositionMeters();
    const FVector DeltaMeters = Solution.ProbePositionMeters - StartPositionMeters;
    if (!TestTrue(TEXT("Approach velocity accepted"),
        Adapter->CommandSetVelocityMetersPerSecond(DeltaMeters).bAccepted)) return false;
    if (!Fixture.TickFor(1.0f, this)) return false;
    if (!TestTrue(TEXT("Velocity stops at the staging position"),
        Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector).bAccepted)) return false;
    const double StagingErrorMeters = (Adapter->GetProbePositionMeters() - Solution.ProbePositionMeters).Size();
    if (!TestTrue(TEXT("Probe reaches the solved staging position"), StagingErrorMeters < 0.05)) return false;

    // 5. Align the arm exactly as the player's align control does. ManipulatorRig's hull guard rejects a whole slew
    // frame the instant a candidate pose would sweep through the probe's hull, so a direct three-joint command stalls
    // the shoulder near 10 degrees on its way to the solved ~50 (reproduced in manipulator_path_planner_tests). The
    // adapter plans collision-free waypoints from the arm's current angles under the live guard; each is commanded
    // and confirmed converged before the next. 10 s per waypoint bounds even a full 360-degree wrist swing at the
    // real 45 deg/s slew (manipulator.hpp's kJointSlewDegreesPerSecond).
    TArray<FVector> Path;
    if (!TestTrue(TEXT("A collision-free joint path to the solved mining pose exists"),
        Adapter->PlanArmPathToDegrees(
            Arm, Solution.ShoulderDegrees, Solution.ElbowDegrees, Solution.WristDegrees, Path))) return false;
    for (const FVector& Waypoint : Path)
    {
        if (!TestTrue(TEXT("Waypoint shoulder target accepted"),
            Adapter->CommandSetManipulatorJointTargetDegrees(
                Arm, EEverwardManipulatorJoint::Shoulder, Waypoint.X).bAccepted)) return false;
        if (!TestTrue(TEXT("Waypoint elbow target accepted"),
            Adapter->CommandSetManipulatorJointTargetDegrees(
                Arm, EEverwardManipulatorJoint::Elbow, Waypoint.Y).bAccepted)) return false;
        if (!TestTrue(TEXT("Waypoint wrist target accepted"),
            Adapter->CommandSetManipulatorJointTargetDegrees(
                Arm, EEverwardManipulatorJoint::Wrist, Waypoint.Z).bAccepted)) return false;
        bool bArrived = false;
        for (int32 Chunk = 0; Chunk < 20 && !bArrived; ++Chunk)
        {
            if (!Fixture.TickFor(0.5f, this)) return false;
            if (!TestTrue(TEXT("Arm state readable while following the planned path"), ArmState(State))) return false;
            bArrived = FMath::Abs(State.ShoulderDegrees - Waypoint.X) <= 0.01 &&
                FMath::Abs(State.ElbowDegrees - Waypoint.Y) <= 0.01 &&
                FMath::Abs(State.WristDegrees - Waypoint.Z) <= 0.01;
        }
        if (!TestTrue(TEXT("Arm reaches each planned waypoint, not stuck on the hull guard"), bArrived)) return false;
    }
    if (!TestTrue(TEXT("Final arm state readable"), ArmState(State))) return false;
    if (!TestTrue(TEXT("Arm ends on the solved mining pose"),
        FMath::Abs(State.ShoulderDegrees - Solution.ShoulderDegrees) <= 0.5 &&
        FMath::Abs(State.ElbowDegrees - Solution.ElbowDegrees) <= 0.5 &&
        FMath::Abs(State.WristDegrees - Solution.WristDegrees) <= 0.5)) return false;

    // 6. The actual G01 success assertion FEverwardG03RejectedMiningTest deliberately
    // stops short of: a real accepted mine_once with positive named-material inventory
    // and storage delta, a reduced deposit, and visible unambiguous feedback -- not
    // just a non-crash.
    const FEverwardMiningStatus Before = Adapter->GetMiningStatus();
    const FEverwardProbeTelemetry TelemetryBefore = Adapter->GetProbeTelemetry();

    const FEverwardProbeCommandResult Attempt = Adapter->CommandMineBootstrapTarget();
    if (!TestTrue(TEXT("Surveyed, tooled, in-reach mining is accepted"), Attempt.bAccepted))
    {
        AddError(FString::Printf(TEXT("Mining rejected: %s"), *Attempt.Detail));
        return false;
    }
    TestTrue(TEXT("Accepted mining reports visible, human-readable feedback"),
        Attempt.Detail.Contains(TEXT("MINED")));

    const FEverwardMiningStatus After = Adapter->GetMiningStatus();
    const FEverwardProbeTelemetry TelemetryAfter = Adapter->GetProbeTelemetry();
    TestEqual(TEXT("Exactly one extraction cycle's worth of material is credited"),
        After.ExtractedMaterialKilograms - Before.ExtractedMaterialKilograms, Before.ExtractionKilogramsPerCycle);
    TestEqual(TEXT("The deposit shrinks by the same amount that was extracted"),
        Before.DepositRemainingKilograms - After.DepositRemainingKilograms, Before.ExtractionKilogramsPerCycle);
    TestTrue(TEXT("Storage actually increases, not just the mining status widget"),
        TelemetryAfter.StorageUsedKilograms > TelemetryBefore.StorageUsedKilograms);

    const TArray<FEverwardMaterialInventoryEntry> Inventory = Adapter->GetStoredMaterialInventory();
    bool bFoundMinedMaterial = false;
    for (const FEverwardMaterialInventoryEntry& Entry : Inventory)
    {
        bFoundMinedMaterial |= Entry.Kilograms > 0.0;
    }
    TestTrue(TEXT("Mined mass lands in named inventory, not an anonymous number"), bFoundMinedMaterial);
    TestFalse(TEXT("Automation notice records acceptance, not rejection"),
        Adapter->GetLastAutomationNotice().bRejected);

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
// that the adapter forwards to them and mirrors the result. The damage-then-repair completion path is covered by
// the portable ctest objective_cycle suite and in-engine by RestorationObjectiveCycleCompletes below.
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

// First-playable-loop step 9, completion path: damage then repair drives the mirrored cycle to Completed in-engine.
// Damage and repair both go through the authoritative integrity setter (TestOnlySetSubsystemIntegrity, compiled out
// of shipping builds); the cycle rules themselves stay in simulation. PENDING until run in Unreal.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03RestorationObjectiveCycleCompletesTest,
    "Everward.G03.Native.RestorationObjectiveCycleCompletes",
    EverwardNativeFlags)

bool FEverwardG03RestorationObjectiveCycleCompletesTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;

    if (!TestTrue(TEXT("Trigger starts the first cycle"),
        Adapter->CommandTriggerNextRestorationCycle().bAccepted)) return false;
    FEverwardRestorationObjectiveCycleStatus Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Cycle is Active"), Cycle.State == EEverwardRestorationObjectiveCycleState::Active);
    TestFalse(TEXT("No damage latched before damage"), Cycle.bDamageObservedThisCycle);

    // Damage a subsystem: starter restoration becomes unsatisfied, the latch sets, the cycle stays Active.
    if (!TestTrue(TEXT("Damage hook accepted"),
        Adapter->TestOnlySetSubsystemIntegrity(EEverwardPowerSubsystem::Thermal, 0.5).bAccepted)) return false;
    if (!Fixture.TickFor(0.20f, this)) return false;
    TestFalse(TEXT("Damaged probe is not restored"), Adapter->GetStarterRestorationStatus().bSatisfied);
    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Cycle stays Active while damaged"), Cycle.State == EEverwardRestorationObjectiveCycleState::Active);
    TestTrue(TEXT("Damage latch is set"), Cycle.bDamageObservedThisCycle);
    TestEqual(TEXT("Nothing completed while damaged"), Cycle.CompletedCycles, 0);

    // Repair: the probe is restored again, so the cycle completes exactly once.
    if (!TestTrue(TEXT("Repair hook accepted"),
        Adapter->TestOnlySetSubsystemIntegrity(EEverwardPowerSubsystem::Thermal, 1.0).bAccepted)) return false;
    if (!Fixture.TickFor(0.20f, this)) return false;
    TestTrue(TEXT("Repaired probe is restored"), Adapter->GetStarterRestorationStatus().bSatisfied);
    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Cycle is Completed after repair"), Cycle.State == EEverwardRestorationObjectiveCycleState::Completed);
    TestEqual(TEXT("Exactly one cycle completed"), Cycle.CompletedCycles, 1);
    if (!Fixture.TickFor(0.20f, this)) return false;
    TestEqual(TEXT("Completed does not count again while ticking"),
        Adapter->GetRestorationObjectiveCycleStatus().CompletedCycles, 1);

    // The next trigger starts a fresh Active cycle with the latch cleared and the count kept.
    if (!TestTrue(TEXT("Next trigger accepted after Completed"),
        Adapter->CommandTriggerNextRestorationCycle().bAccepted)) return false;
    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Fresh cycle is Active"), Cycle.State == EEverwardRestorationObjectiveCycleState::Active);
    TestFalse(TEXT("Fresh cycle clears the damage latch"), Cycle.bDamageObservedThisCycle);
    TestEqual(TEXT("Completed count is kept"), Cycle.CompletedCycles, 1);
    return !HasAnyErrors();
}

// First-playable-loop step 9b: the in-engine mirror of the engine-independent progression reward
// (src/simulation/include/everward/simulation/progression_reward.hpp). Rules stay in simulation; this only checks
// that the adapter forwards to them and mirrors the result once the same damage-then-repair completion path
// RestorationObjectiveCycleCompletes above exercises actually completes a cycle.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03ProgressionRewardMirrorTest,
    "Everward.G03.Native.ProgressionRewardMirror",
    EverwardNativeFlags)

bool FEverwardG03ProgressionRewardMirrorTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;

    FEverwardProgressionRewardStatus Reward = Adapter->GetProgressionRewardStatus();
    TestEqual(TEXT("No reward before any cycle completes"), Reward.RewardedCycles, 0);
    TestEqual(TEXT("No reward material stored yet"), Reward.MaterialKilograms, 0.0);
    TestTrue(TEXT("Reward mirror names a material id even before any payout"), !Reward.MaterialId.IsEmpty());
    TestTrue(TEXT("Nothing fabricated: inventory starts empty"),
        Adapter->GetStoredMaterialInventory().IsEmpty());

    // Drive one cycle to completion exactly as RestorationObjectiveCycleCompletes does above.
    if (!TestTrue(TEXT("Trigger starts the first cycle"),
        Adapter->CommandTriggerNextRestorationCycle().bAccepted)) return false;
    if (!TestTrue(TEXT("Damage hook accepted"),
        Adapter->TestOnlySetSubsystemIntegrity(EEverwardPowerSubsystem::Thermal, 0.5).bAccepted)) return false;
    if (!Fixture.TickFor(0.20f, this)) return false;
    if (!TestTrue(TEXT("Repair hook accepted"),
        Adapter->TestOnlySetSubsystemIntegrity(EEverwardPowerSubsystem::Thermal, 1.0).bAccepted)) return false;
    if (!Fixture.TickFor(0.20f, this)) return false;

    FEverwardRestorationObjectiveCycleStatus Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Cycle is Completed after repair"), Cycle.State == EEverwardRestorationObjectiveCycleState::Completed);
    TestEqual(TEXT("Exactly one cycle completed"), Cycle.CompletedCycles, 1);

    Reward = Adapter->GetProgressionRewardStatus();
    TestEqual(TEXT("Exactly one reward paid for the one completed cycle"), Reward.RewardedCycles, 1);
    TestEqual(TEXT("Reward kilograms match progression_reward.hpp's bounded unit"), Reward.MaterialKilograms, 1.0);

    // The mirror reads the same authoritative inventory GetStoredMaterialInventory() already exposes, not a
    // second notion of "how much material exists".
    const TArray<FEverwardMaterialInventoryEntry> Inventory = Adapter->GetStoredMaterialInventory();
    bool bFoundRewardMaterial = false;
    for (const FEverwardMaterialInventoryEntry& Entry : Inventory)
    {
        if (Entry.MaterialId == Reward.MaterialId)
        {
            bFoundRewardMaterial = true;
            TestEqual(TEXT("Inventory entry matches the reward mirror's kilograms"),
                Entry.Kilograms, Reward.MaterialKilograms);
        }
    }
    TestTrue(TEXT("Rewarded material actually appears in storage"), bFoundRewardMaterial);

    // Ticking further without another completed cycle must not pay a second reward.
    if (!Fixture.TickFor(0.20f, this)) return false;
    TestEqual(TEXT("No double-pay while the cycle stays Completed"),
        Adapter->GetProgressionRewardStatus().RewardedCycles, 1);

    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03SelectedTargetScanTest,
    "Everward.G03.Native.SelectedTargetScanBinding",
    EverwardNativeFlags)

bool FEverwardG03SelectedTargetScanTest::RunTest(const FString& Parameters)
{
    FEverwardNativeTestFixture Fixture;
    if (!Fixture.Start(this)) return false;
    UProbeSimulationAdapter* Adapter = Fixture.Adapter;
    const FString RefA = AEverwardPhase2TestEnvironment::ReferenceTarget1Id;
    const FString RefB = AEverwardPhase2TestEnvironment::ReferenceTarget2Id;
    const FString Bootstrap = AEverwardPhase2TestEnvironment::BootstrapScanTargetId;

    TestFalse(TEXT("Bootstrap begins without a completed resource survey"),
        Adapter->GetMiningStatus().bSurveyed);
    (void)Adapter->CommandClearTargetSelection();
    TestFalse(TEXT("No selection is observable"), Adapter->GetSelectedTargetStatus().bHasSelection);
    const FEverwardProbeCommandResult NoTarget = Adapter->CommandStartScan(FString(), 0.25);
    TestFalse(TEXT("Empty selected target is rejected without scan"), NoTarget.bAccepted);
    TestEqual(TEXT("No-selection rejection is actionable"),
        NoTarget.Detail, FString(TEXT("no target selected")));
    TestFalse(TEXT("No selection creates no active scan"), Adapter->GetProbeTelemetry().bIsScanning);
    const FEverwardProbeCommandResult Missing = Adapter->CommandStartScan(
        TEXT("phase2-not-registered"), 0.25);
    TestFalse(TEXT("Unknown physical body cannot be scanned"), Missing.bAccepted);
    TestTrue(TEXT("Unknown body rejection names the registration problem"),
        Missing.Detail.Contains(TEXT("not registered")));
    TestFalse(TEXT("Unknown target cannot create an active scan"),
        Adapter->GetProbeTelemetry().bIsScanning);

    if (!TestTrue(TEXT("Sensors receive the required power"),
        Adapter->CommandAllocatePower(EEverwardPowerSubsystem::Sensors, 100.0).bAccepted)) return false;
    if (!TestTrue(TEXT("Reference A selected through actual adapter"),
        Adapter->CommandSelectTarget(RefA).bAccepted)) return false;
    if (!TestTrue(TEXT("Selected reference A scan accepted"),
        Adapter->CommandStartScan(Adapter->GetSelectedTargetStatus().TargetId, 0.25).bAccepted)) return false;
    TestEqual(TEXT("Scan is bound to reference A, not the bootstrap"),
        Adapter->GetProbeTelemetry().ActiveScanTargetId, RefA);

    if (!TestTrue(TEXT("Reference B can be selected during A's scan"),
        Adapter->CommandSelectTarget(RefB).bAccepted)) return false;
    TestEqual(TEXT("Selection changing to B does not retarget the running scan"),
        Adapter->GetProbeTelemetry().ActiveScanTargetId, RefA);
    const FEverwardProbeCommandResult Reissue = Adapter->CommandStartScan(RefB, 0.25);
    TestFalse(TEXT("Reissue is rejected until explicit cancellation"), Reissue.bAccepted);
    TestTrue(TEXT("Existing in-progress rejection is preserved"),
        Reissue.Detail.Contains(TEXT("already in progress")));
    TestEqual(TEXT("Rejected scan cannot silently retarget A to B"),
        Adapter->GetProbeTelemetry().ActiveScanTargetId, RefA);

    if (!Fixture.TickFor(0.80f, this)) return false;
    TestTrue(TEXT("Real completed event registered"), Adapter->GetLastScanLifecycleNotice().bCompleted);
    TestEqual(TEXT("Scan A identity survives even though no HUD frame was drawn during the scan"),
        Adapter->GetLastScanLifecycleNotice().TargetId, RefA);
    TestEqual(TEXT("Retained completion identifies scanned A"),
        Adapter->GetLastCompletedScanLifecycleNotice().TargetId, RefA);
    TestTrue(TEXT("Completed event belongs to scanned A"),
        Adapter->GetLastScanLifecycleNotice().Detail.Contains(RefA));
    TestFalse(TEXT("Scanning reference A does not survey the bootstrap"),
        Adapter->GetMiningStatus().bSurveyed);
    const FEverwardTargetKnowledgeStatus BKnowledge = Adapter->GetSelectedTargetKnowledgeStatus();
    TestTrue(TEXT("Selection B does not receive active-scan seconds from A"),
        !BKnowledge.bHasKnowledge || FMath::IsNearlyZero(BKnowledge.ActiveScanSeconds));
    if (!TestTrue(TEXT("Re-selecting A succeeds"), Adapter->CommandSelectTarget(RefA).bAccepted)) return false;
    const FEverwardTargetKnowledgeStatus AKnowledge = Adapter->GetSelectedTargetKnowledgeStatus();
    TestTrue(TEXT("Actual A has positive active-scan evidence"),
        AKnowledge.bHasKnowledge && AKnowledge.ActiveScanSeconds > 0.0);
    TestTrue(TEXT("Reference A is not falsely classified"),
        AKnowledge.Classification.IsEmpty() &&
        AKnowledge.Level != EEverwardKnowledgeLevel::Characterized);

    if (!TestTrue(TEXT("Bootstrap can still be selected"), Adapter->CommandSelectTarget(Bootstrap).bAccepted)) return false;
    if (!TestTrue(TEXT("Selected bootstrap scan starts"), Adapter->CommandStartScan(
        Adapter->GetSelectedTargetStatus().TargetId, 0.25).bAccepted)) return false;
    if (!Fixture.TickFor(0.80f, this)) return false;
    TestTrue(TEXT("Completed bootstrap scan still earns the resource survey"),
        Adapter->GetMiningStatus().bSurveyed);
    TestFalse(TEXT("A newly completed scan is not a restored bookmark"),
        Adapter->GetLastCompletedScanLifecycleNotice().bRestoredFromSave);
    if (!TestTrue(TEXT("Save after completed bootstrap survey"),
        Adapter->CommandSaveGame().bAccepted)) return false;

    if (!TestTrue(TEXT("Switch to reference B"), Adapter->CommandSelectTarget(RefB).bAccepted)) return false;
    if (!TestTrue(TEXT("Start a cancellable reference scan"), Adapter->CommandStartScan(RefB, 5.0).bAccepted)) return false;
    if (!TestTrue(TEXT("Change selection while scan runs"), Adapter->CommandSelectTarget(RefA).bAccepted)) return false;
    if (!TestTrue(TEXT("Explicit cancel always acts on bound scan"),
        Adapter->CommandCancelScan().bAccepted)) return false;
    if (!Fixture.TickFor(0.10f, this)) return false;
    TestTrue(TEXT("Cancelled scan sends its own domain event"),
        Adapter->GetLastScanLifecycleNotice().bCancelled &&
        Adapter->GetLastScanLifecycleNotice().Detail.Contains(RefB));
    TestEqual(TEXT("Explicit cancellation reports the original bound target"),
        Adapter->GetLastScanLifecycleNotice().TargetId, RefB);
    TestFalse(TEXT("Cancelled scan cannot keep running"), Adapter->GetProbeTelemetry().bIsScanning);
    TestTrue(TEXT("Prior bootstrap survey survives unrelated scan cancellation"),
        Adapter->GetMiningStatus().bSurveyed);
    TestEqual(TEXT("Cancelled reference scan cannot replace the last completed bootstrap discovery"),
        Adapter->GetLastCompletedScanLifecycleNotice().TargetId, Bootstrap);

    if (!TestTrue(TEXT("Load restores the earlier saved survey"),
        Adapter->CommandLoadGame().bAccepted)) return false;
    TestTrue(TEXT("Completed bootstrap survey survives save/load"),
        Adapter->GetMiningStatus().bSurveyed);
    TestEqual(TEXT("Last completed scan bookmark survives save/load after cancelled scan"),
        Adapter->GetLastCompletedScanLifecycleNotice().TargetId, Bootstrap);
    TestTrue(TEXT("Loaded completion is identified as restored, not newly acquired"),
        Adapter->GetLastCompletedScanLifecycleNotice().bRestoredFromSave);
    TestFalse(TEXT("Load discards a session-local cancelled scan notice"),
        Adapter->GetLastScanLifecycleNotice().bCancelled);

    if (!TestTrue(TEXT("Start reference scan before sensor-power interruption"),
        Adapter->CommandStartScan(RefB, 5.0).bAccepted)) return false;
    if (!TestTrue(TEXT("Dropping sensor power is an accepted player action"),
        Adapter->CommandAllocatePower(EEverwardPowerSubsystem::Sensors, 0.0).bAccepted)) return false;
    TestFalse(TEXT("Below-minimum sensor power cancels the active scan"),
        Adapter->GetProbeTelemetry().bIsScanning);
    if (!Fixture.TickFor(0.10f, this)) return false;
    TestTrue(TEXT("Power-loss cancellation recorded as cancellation, not completion"),
        Adapter->GetLastScanLifecycleNotice().bCancelled);
    TestEqual(TEXT("Power cancellation reports its original bound target"),
        Adapter->GetLastScanLifecycleNotice().TargetId, RefB);
    TestFalse(TEXT("Cannot start a new scan while sensors have no power"),
        Adapter->CommandStartScan(RefB, 0.25).bAccepted);

    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
