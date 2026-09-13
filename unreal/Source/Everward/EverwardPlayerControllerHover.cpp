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
    LastSeenControlledHoverGovernorSequence = Adapter->GetControlledHoverGovernorNotice().Sequence;
    Adapter->SetControlledHoverGovernorEngaged(
        true,
        ControlledHoverMaxTangentialSpeedMetersPerSecond,
        ControlledHoverMinimumClearanceMeters,
        ControlledHoverTargetAltitudeMeters,
        ControlledHoverAltitudeGainPerSecond,
        ControlledHoverMaxVerticalCorrectionSpeedMetersPerSecond);
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
    // through the exact command
    // surface_descent_guidance.hpp/GetControlledHoverVelocityCommand()
    // already established (docs/PHASE2_SURFACE_HOVER_COMMAND_TEST.md), so
    // ordinary WASDQE trim still steers laterally while this only corrects
    // the radial component toward the configured target altitude. The
    // correction itself is re-applied once per elapsed authoritative fixed
    // step by UProbeSimulationAdapter::AdvanceControlledHoverGovernorFixedStep()
    // -- called from inside TickComponent()'s own fixed-step accumulator loop
    // -- rather than here once per render frame (issue #235 finding 2); this
    // only detects a fixed-step rejection so the player-facing toggle/HUD
    // state stays in sync.
    const FEverwardAutomationNotice Notice = Adapter->GetControlledHoverGovernorNotice();
    if (Notice.Sequence != LastSeenControlledHoverGovernorSequence)
    {
        LastSeenControlledHoverGovernorSequence = Notice.Sequence;
        if (Notice.bRejected)
        {
            bControlledHoverEngaged = false;
            ShowHoverMessage(FString::Printf(TEXT("Controlled hover stopped: %s"), *Notice.Detail), FColor::Orange);
        }
    }
}

void AEverwardPlayerController::CancelControlledHover(bool bStopVelocity, bool bShowMessage)
{
    const bool bWasEngaged = bControlledHoverEngaged;
    bControlledHoverEngaged = false;

    if (UProbeSimulationAdapter* Adapter = GetProbeAdapter())
    {
        // Stop the fixed-step correction immediately rather than leaving it
        // engaged on the adapter until a rejection happens to occur.
        Adapter->SetControlledHoverGovernorEngaged(false, 0.0, 0.0, 0.0, 0.0, 0.0);
        if (bStopVelocity)
        {
            (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        }
    }

    if (bShowMessage && bWasEngaged)
    {
        ShowHoverMessage(TEXT("Controlled hover released. Manual control restored."));
    }
}
