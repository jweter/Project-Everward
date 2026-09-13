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
    CancelControlledHover(false, false);
    bAutoApproachMiningTarget = false;
    bControlledDescentEngaged = true;
    LastSeenControlledDescentGovernorSequence = Adapter->GetControlledDescentGovernorNotice().Sequence;
    Adapter->SetControlledDescentGovernorEngaged(
        true,
        ControlledDescentMaxDescentSpeedMetersPerSecond,
        ControlledDescentMaxTangentialSpeedMetersPerSecond,
        ControlledDescentMinimumClearanceMeters,
        ControlledDescentFullSpeedAltitudeMeters,
        ControlledDescentTouchdownSpeedMetersPerSecond);
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
    // The correction itself is re-applied once per elapsed authoritative
    // fixed step by UProbeSimulationAdapter::AdvanceControlledDescentGovernorFixedStep()
    // -- called from inside TickComponent()'s own fixed-step accumulator
    // loop, the same pattern issue #235/#238 established for controlled
    // hover -- rather than here once per render frame (issue #239); this
    // only detects a fixed-step rejection so the player-facing toggle/HUD
    // state stays in sync.
    const FEverwardAutomationNotice Notice = Adapter->GetControlledDescentGovernorNotice();
    if (Notice.Sequence != LastSeenControlledDescentGovernorSequence)
    {
        LastSeenControlledDescentGovernorSequence = Notice.Sequence;
        if (Notice.bRejected)
        {
            bControlledDescentEngaged = false;
            ShowDescentMessage(FString::Printf(TEXT("Controlled descent stopped: %s"), *Notice.Detail), FColor::Orange);
        }
    }
}

void AEverwardPlayerController::CancelControlledDescent(bool bStopVelocity, bool bShowMessage)
{
    const bool bWasEngaged = bControlledDescentEngaged;
    bControlledDescentEngaged = false;

    if (UProbeSimulationAdapter* Adapter = GetProbeAdapter())
    {
        // Stop the fixed-step correction immediately rather than leaving it
        // engaged on the adapter until a rejection happens to occur.
        Adapter->SetControlledDescentGovernorEngaged(false, 0.0, 0.0, 0.0, 0.0, 0.0);
        if (bStopVelocity)
        {
            (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        }
    }

    if (bShowMessage && bWasEngaged)
    {
        ShowDescentMessage(TEXT("Controlled descent released. Manual control restored."));
    }
}
