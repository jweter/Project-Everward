// First-playable-loop step 9 HUD/trigger wiring: proves that
// AEverwardPlayerController's F7 input handler (TriggerNextRestorationCycle(), reached here through a
// WITH_DEV_AUTOMATION_TESTS-only friend declaration the same way
// UProbeSimulationAdapter::TestOnlySetSubsystemIntegrity() is a test-only hook) drives
// UProbeSimulationAdapter::CommandTriggerNextRestorationCycle() -- the exact command the HUD's [F7]
// trigger hint promises -- and that the resulting mirror (GetRestorationObjectiveCycleStatus(),
// GetStarterRestorationStatus(), GetLastCommandResult()) is exactly what EverwardHUD::DrawHUD() reads
// each frame. No cycle rule, HUD text, or rejection message is reimplemented here; see
// EverwardG03NativeTests.cpp's RestorationObjectiveCycleMirror/Completes tests for the adapter-level
// coverage this test does not duplicate. No Aura.
#include "EverwardPlayerController.h"
#include "EverwardProbePawn.h"
#include "ProbeSimulationAdapter.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEverwardG03RestorationCycleHudTriggerTest,
    "Everward.G03.Native.RestorationObjectiveCycleHudTrigger",
    EAutomationTestFlags::EditorContext |
    EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::CommandletContext |
    EAutomationTestFlags::ProductFilter)

bool FEverwardG03RestorationCycleHudTriggerTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper World;
    if (!World.CreateTestWorld(EWorldType::Game))
    {
        World.ForwardErrorMessages(this);
        return false;
    }
    UWorld* GameWorld = World.GetTestWorld();
    if (!GameWorld)
    {
        AddError(TEXT("Native HUD trigger test game world could not be created"));
        return false;
    }

    AEverwardProbePawn* Pawn = GameWorld->SpawnActor<AEverwardProbePawn>(
        AEverwardProbePawn::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Pawn)
    {
        AddError(TEXT("Native HUD trigger test probe actor could not be spawned"));
        return false;
    }
    AEverwardPlayerController* Controller = GameWorld->SpawnActor<AEverwardPlayerController>();
    if (!Controller)
    {
        AddError(TEXT("Native HUD trigger test player controller could not be spawned"));
        return false;
    }

    if (!World.BeginPlayInTestWorld())
    {
        World.ForwardErrorMessages(this);
        return false;
    }

    UProbeSimulationAdapter* Adapter = Pawn->GetSimulationAdapter();
    if (!TestTrue(TEXT("Probe has a simulation adapter"), Adapter != nullptr))
    {
        return false;
    }

    Controller->Possess(Pawn);
    if (!TestTrue(TEXT("Controller possesses the probe pawn"), Controller->GetPawn() == Pawn))
    {
        return false;
    }

    // Before any trigger: Idle, matching what the HUD's OBJECTIVE row and [F7] trigger hint would show.
    FEverwardRestorationObjectiveCycleStatus Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Cycle starts Idle"), Cycle.State == EEverwardRestorationObjectiveCycleState::Idle);
    TestEqual(TEXT("No cycle completed yet"), Cycle.CompletedCycles, 0);

    // Drive the exact bound input handler F7 invokes -- not the adapter command directly -- so this
    // test actually exercises the controller binding rather than restating the adapter-level coverage.
    Controller->TriggerNextRestorationCycle();

    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Controller binding starts the cycle: mirror reads Active"),
        Cycle.State == EEverwardRestorationObjectiveCycleState::Active);
    const FEverwardProbeCommandResult Accepted = Adapter->GetLastCommandResult();
    TestTrue(TEXT("Accepted trigger is recorded on the same LastCommandResult path the HUD banner reads"),
        Accepted.bAccepted);

    // A fresh, undamaged probe has nothing pending -- the HUD's PENDING list and alert color both
    // depend on this staying empty while Active.
    const FEverwardStarterRestorationStatus Restoration = Adapter->GetStarterRestorationStatus();
    TestTrue(TEXT("Fresh probe has no pending subsystems"), Restoration.PendingSubsystems.IsEmpty());

    // Re-pressing F7 while Active must be rejected, and that rejection must land on the same
    // LastCommandResult the HUD's "COMMAND REJECTED" banner already reads -- no second rejection path.
    Controller->TriggerNextRestorationCycle();
    Cycle = Adapter->GetRestorationObjectiveCycleStatus();
    TestTrue(TEXT("Re-trigger while Active leaves the mirror Active"),
        Cycle.State == EEverwardRestorationObjectiveCycleState::Active);
    const FEverwardProbeCommandResult Rejected = Adapter->GetLastCommandResult();
    TestFalse(TEXT("Re-trigger while Active is rejected on the HUD-facing LastCommandResult"), Rejected.bAccepted);
    TestTrue(TEXT("Rejection sequence advanced so the HUD banner would show it"),
        Rejected.Sequence > Accepted.Sequence);

    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
