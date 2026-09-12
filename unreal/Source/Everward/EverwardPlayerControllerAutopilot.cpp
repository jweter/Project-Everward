#include "EverwardPlayerController.h"

#include "Engine/Engine.h"
#include "ProbeSimulationAdapter.h"

namespace
{
void ShowJoseMessage(const FString& Message, const FColor& Color = FColor::Cyan)
{
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, Color, Message);
    }
}
}

void AEverwardPlayerController::ToggleJoseTakeTheWheel()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    if (bJoseAutopilotEngaged)
    {
        CancelJoseTakeTheWheel(true, true);
        return;
    }

    const FEverwardTargetSelectionStatus Target = Adapter->GetSelectedTargetStatus();
    if (!Target.bHasSelection)
    {
        ShowJoseMessage(TEXT("José needs a destination. Press T to select a physical target, then press Y."), FColor::Orange);
        return;
    }

    FVector DestinationMeters;
    if (!Adapter->GetStaticBodyPositionMeters(Target.TargetId, DestinationMeters))
    {
        ShowJoseMessage(TEXT("José cannot resolve that destination right now."), FColor::Orange);
        return;
    }

    CancelControlledDescent(false, false);
    CancelControlledHover(false, false);
    bAutoApproachMiningTarget = false;
    bJoseAutopilotEngaged = true;
    JoseDestinationTargetId = Target.TargetId;

    ShowJoseMessage(FString::Printf(
        TEXT("José Take the Wheel // destination %s // autopilot engaged // SPACE or manual thrust returns control"),
        *JoseDestinationTargetId));
}

void AEverwardPlayerController::AdvanceJoseTakeTheWheel(float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (!bJoseAutopilotEngaged)
    {
        return;
    }

    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        CancelJoseTakeTheWheel(false, false);
        return;
    }

    const FEverwardTargetSelectionStatus Target = Adapter->GetSelectedTargetStatus();
    if (!Target.bHasSelection || Target.TargetId != JoseDestinationTargetId)
    {
        CancelJoseTakeTheWheel(true, false);
        ShowJoseMessage(TEXT("José released the wheel because the selected destination changed."), FColor::Orange);
        return;
    }

    // Phase-2 José is deliberately simple but physically useful: aim at the
    // selected body's live center, use the simulation's authoritative surface
    // range as the arrival metric, and progressively reduce commanded speed as
    // the safe stand-off is approached. The guidance decision itself lives in
    // the engine-independent jose_autopilot.hpp (see GetJoseGuidanceCommand()),
    // not here -- this controller only interprets the resulting Outcome and
    // issues/cancels the authoritative velocity command. Later versions can
    // replace that local guidance law with orbital intercepts, obstacle
    // avoidance, route planning, power/thermal budgeting, and interplanetary
    // navigation without changing the player-facing "select destination ->
    // José" contract.
    const FEverwardJoseGuidanceCommand Guidance = Adapter->GetJoseGuidanceCommand(
        JoseDestinationTargetId,
        JoseCruiseSpeedMetersPerSecond,
        JoseArrivalSurfaceRangeMeters,
        JoseArrivalToleranceMeters,
        JoseApproachGainPerSecond);
    switch (Guidance.Outcome)
    {
        case EEverwardJoseGuidanceOutcome::DestinationNotFound:
        {
            CancelJoseTakeTheWheel(true, false);
            ShowJoseMessage(TEXT("José released the wheel because the destination is no longer available."), FColor::Orange);
            return;
        }
        case EEverwardJoseGuidanceOutcome::Arrived:
        {
            const FString ArrivedAt = JoseDestinationTargetId;
            CancelJoseTakeTheWheel(true, false);
            ShowJoseMessage(FString::Printf(
                TEXT("José Take the Wheel // arrived at %s // holding %.1f m surface standoff"),
                *ArrivedAt,
                JoseArrivalSurfaceRangeMeters));
            return;
        }
        case EEverwardJoseGuidanceOutcome::DestinationUnresolved:
        {
            CancelJoseTakeTheWheel(true, false);
            ShowJoseMessage(TEXT("José stopped: destination geometry is unresolved."), FColor::Orange);
            return;
        }
        case EEverwardJoseGuidanceOutcome::Continue:
        default:
            break;
    }

    const FEverwardProbeCommandResult Result = Adapter->CommandSetVelocityMetersPerSecond(Guidance.CommandVelocityMetersPerSecond);
    if (!Result.bAccepted)
    {
        CancelJoseTakeTheWheel(false, false);
        ShowJoseMessage(FString::Printf(TEXT("José autopilot stopped: %s"), *Result.Detail), FColor::Orange);
    }
}

void AEverwardPlayerController::CancelJoseTakeTheWheel(bool bStopVelocity, bool bShowMessage)
{
    const bool bWasEngaged = bJoseAutopilotEngaged;
    bJoseAutopilotEngaged = false;
    JoseDestinationTargetId.Reset();

    if (bStopVelocity)
    {
        if (UProbeSimulationAdapter* Adapter = GetProbeAdapter())
        {
            (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        }
    }

    if (bShowMessage && bWasEngaged)
    {
        ShowJoseMessage(TEXT("José released the wheel. Manual control restored."));
    }
}
