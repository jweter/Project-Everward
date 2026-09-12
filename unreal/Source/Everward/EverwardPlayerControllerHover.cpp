#include "EverwardPlayerController.h"

#include "Engine/Engine.h"
#include "ProbeSimulationAdapter.h"

namespace
{
void ShowHoverMessage(const FString& Message, const FColor& Color = FColor::Cyan)
{
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, Color, Message);
    }
}
}

void AEverwardPlayerController::ToggleControlledHover()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    if (bControlledHoverEngaged)
    {
        CancelControlledHover(true, true);
        return;
    }

    const FEverwardControlledDescentCommand Preview = Adapter->GetControlledHoverVelocityCommand(
        Adapter->GetProbeTelemetry().VelocityMetersPerSecond,
        ControlledHoverMaxTangentialSpeedMetersPerSecond,
        ControlledHoverMinimumClearanceMeters,
        ControlledHoverTargetAltitudeMeters,
        ControlledHoverAltitudeGainPerSecond,
        ControlledHoverMaxVerticalCorrectionSpeedMetersPerSecond);
    if (!Preview.bHasResult)
    {
        ShowHoverMessage(TEXT("Controlled hover needs a registered planetary body nearby."), FColor::Orange);
        return;
    }

    CancelJoseTakeTheWheel(false, false);
    CancelControlledDescent(false, false);
    bAutoApproachMiningTarget = false;
    bControlledHoverEngaged = true;
    ShowHoverMessage(TEXT(
        "Controlled hover engaged // holding altitude near the surface // SPACE or manual thrust returns control"));
}

void AEverwardPlayerController::AdvanceControlledHover(float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (!bControlledHoverEngaged)
    {
        return;
    }

    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        CancelControlledHover(false, false);
        return;
    }

    // Hover is a velocity governor like controlled descent, not a
    // destination autopilot: it re-shapes the probe's own current velocity
    // every fixed step through the exact command
    // surface_descent_guidance.hpp/GetControlledHoverVelocityCommand()
    // already established (docs/PHASE2_SURFACE_HOVER_COMMAND_TEST.md), so
    // ordinary WASDQE trim still steers laterally while this only corrects
    // the radial component toward the configured target altitude.
    const FEverwardProbeCommandResult Result = Adapter->CommandSetControlledHoverVelocityMetersPerSecond(
        Adapter->GetProbeTelemetry().VelocityMetersPerSecond,
        ControlledHoverMaxTangentialSpeedMetersPerSecond,
        ControlledHoverMinimumClearanceMeters,
        ControlledHoverTargetAltitudeMeters,
        ControlledHoverAltitudeGainPerSecond,
        ControlledHoverMaxVerticalCorrectionSpeedMetersPerSecond);
    if (!Result.bAccepted)
    {
        CancelControlledHover(false, false);
        ShowHoverMessage(FString::Printf(TEXT("Controlled hover stopped: %s"), *Result.Detail), FColor::Orange);
    }
}

void AEverwardPlayerController::CancelControlledHover(bool bStopVelocity, bool bShowMessage)
{
    const bool bWasEngaged = bControlledHoverEngaged;
    bControlledHoverEngaged = false;

    if (bStopVelocity)
    {
        if (UProbeSimulationAdapter* Adapter = GetProbeAdapter())
        {
            (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        }
    }

    if (bShowMessage && bWasEngaged)
    {
        ShowHoverMessage(TEXT("Controlled hover released. Manual control restored."));
    }
}
