#include "EverwardPlayerController.h"

#include "Engine/Engine.h"
#include "ProbeSimulationAdapter.h"

namespace
{
void ShowDescentMessage(const FString& Message, const FColor& Color = FColor::Cyan)
{
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, Color, Message);
    }
}
}

void AEverwardPlayerController::ToggleControlledDescent()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    if (bControlledDescentEngaged)
    {
        CancelControlledDescent(true, true);
        return;
    }

    const FEverwardControlledDescentCommand Preview = Adapter->GetControlledDescentVelocityCommand(
        FVector(0.0, 0.0, -ControlledDescentMaxDescentSpeedMetersPerSecond),
        ControlledDescentMaxDescentSpeedMetersPerSecond,
        ControlledDescentMaxTangentialSpeedMetersPerSecond,
        ControlledDescentMinimumClearanceMeters,
        ControlledDescentFullSpeedAltitudeMeters,
        ControlledDescentTouchdownSpeedMetersPerSecond);
    if (!Preview.bHasResult)
    {
        ShowDescentMessage(TEXT("Controlled descent unavailable: no planetary body is registered."), FColor::Orange);
        return;
    }

    CancelJoseTakeTheWheel(false, false);
    bAutoApproachMiningTarget = false;
    bControlledDescentEngaged = true;
    ShowDescentMessage(TEXT("CONTROLLED DESCENT // engaged // SPACE or manual thrust returns control"));
}

void AEverwardPlayerController::AdvanceControlledDescent(float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (!bControlledDescentEngaged)
    {
        return;
    }

    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        CancelControlledDescent(false, false);
        return;
    }

    const FEverwardProbeCommandResult Result = Adapter->CommandSetControlledDescentVelocityMetersPerSecond(
        FVector(0.0, 0.0, -ControlledDescentMaxDescentSpeedMetersPerSecond),
        ControlledDescentMaxDescentSpeedMetersPerSecond,
        ControlledDescentMaxTangentialSpeedMetersPerSecond,
        ControlledDescentMinimumClearanceMeters,
        ControlledDescentFullSpeedAltitudeMeters,
        ControlledDescentTouchdownSpeedMetersPerSecond);
    if (!Result.bAccepted)
    {
        CancelControlledDescent(false, false);
        ShowDescentMessage(FString::Printf(TEXT("Controlled descent stopped: %s"), *Result.Detail), FColor::Orange);
    }
}

void AEverwardPlayerController::CancelControlledDescent(bool bStopVelocity, bool bShowMessage)
{
    const bool bWasEngaged = bControlledDescentEngaged;
    bControlledDescentEngaged = false;

    if (bStopVelocity)
    {
        if (UProbeSimulationAdapter* Adapter = GetProbeAdapter())
        {
            (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        }
    }

    if (bShowMessage && bWasEngaged)
    {
        ShowDescentMessage(TEXT("Controlled descent released. Manual control restored."));
    }
}
