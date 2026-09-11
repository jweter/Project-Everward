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
        Adapter->GetProbeTelemetry().VelocityMetersPerSecond,
        ControlledDescentMaxDescentSpeedMetersPerSecond,
        ControlledDescentMaxTangentialSpeedMetersPerSecond,
        ControlledDescentMinimumClearanceMeters,
        ControlledDescentFullSpeedAltitudeMeters,
        ControlledDescentTouchdownSpeedMetersPerSecond);
    if (!Preview.bHasResult)
    {
        ShowDescentMessage(TEXT("Controlled descent needs a registered planetary body nearby."), FColor::Orange);
        return;
    }

    CancelJoseTakeTheWheel(false, false);
    bAutoApproachMiningTarget = false;
    bControlledDescentEngaged = true;
    ShowDescentMessage(TEXT(
        "Controlled descent engaged // capping descent/lateral rate near the surface // SPACE or manual thrust returns control"));
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

    // Controlled descent is a velocity governor, not a destination autopilot
    // like José: it re-shapes the probe's own current velocity every fixed
    // step through the exact command surface_descent_guidance.hpp/
    // GetControlledDescentVelocityCommand() already established
    // (docs/PHASE2_SURFACE_DESCENT_COMMAND_TEST.md), so ordinary WASDQE trim
    // still steers while this only caps descent/lateral rate -- tapered by
    // altitude -- as the registered planetary body's surface is approached.
    const FEverwardProbeCommandResult Result = Adapter->CommandSetControlledDescentVelocityMetersPerSecond(
        Adapter->GetProbeTelemetry().VelocityMetersPerSecond,
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
