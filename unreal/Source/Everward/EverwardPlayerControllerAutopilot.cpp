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
    LastSeenJoseAutopilotGovernorSequence = Adapter->GetJoseAutopilotGovernorNotice().Sequence;
    Adapter->SetJoseAutopilotGovernorEngaged(
        true,
        JoseDestinationTargetId,
        JoseCruiseSpeedMetersPerSecond,
        JoseArrivalSurfaceRangeMeters,
        JoseArrivalToleranceMeters,
        JoseApproachGainPerSecond);

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

    // Phase-2 José is deliberately simple but physically useful: aim at the
    // selected body's live center, use the simulation's authoritative surface
    // range as the arrival metric, and progressively reduce commanded speed as
    // the safe stand-off is approached. The guidance decision itself lives in
    // the engine-independent jose_autopilot.hpp, reached through the adapter's
    // read-only guidance query, not here. This controller no longer
    // re-evaluates guidance or issues the velocity command itself -- that ran
    // once per render frame regardless of how many authoritative fixed steps
    // had elapsed (issue #239), the same defect fixed for controlled hover by
    // #238. The correction is now re-applied once per elapsed fixed step by
    // UProbeSimulationAdapter::AdvanceJoseAutopilotGovernorFixedStep() from
    // inside TickComponent()'s own fixed-step accumulator loop; this only
    // detects a fixed-step stop/rejection so the player-facing toggle/HUD
    // state and on-screen message stay in sync. Later versions can replace
    // the local guidance law with orbital intercepts, obstacle avoidance,
    // route planning, power/thermal budgeting, and interplanetary navigation
    // without changing this player-facing "select destination -> José"
    // contract.
    const FEverwardJoseAutopilotGovernorNotice Notice = Adapter->GetJoseAutopilotGovernorNotice();
    if (Notice.Sequence == LastSeenJoseAutopilotGovernorSequence)
    {
        return;
    }
    LastSeenJoseAutopilotGovernorSequence = Notice.Sequence;

    // bStopVelocity is false in every branch below: AdvanceJoseAutopilotGovernorFixedStep()
    // already issued the terminal zero-velocity command synchronously, in the
    // same fixed step that detected the stop (Codex review, PR #245) --
    // re-issuing it here, up to a render frame later, would be redundant at
    // best and would reintroduce the exact render-cadence-dependent stop
    // timing this fix removes. This only resets local toggle/HUD state and
    // shows the corresponding message.
    switch (Notice.StopReason)
    {
        case EEverwardJoseAutopilotStopReason::SelectionChanged:
        {
            CancelJoseTakeTheWheel(false, false);
            ShowJoseMessage(TEXT("José released the wheel because the selected destination changed."), FColor::Orange);
            return;
        }
        case EEverwardJoseAutopilotStopReason::DestinationNotFound:
        {
            CancelJoseTakeTheWheel(false, false);
            ShowJoseMessage(TEXT("José released the wheel because the destination is no longer available."), FColor::Orange);
            return;
        }
        case EEverwardJoseAutopilotStopReason::Arrived:
        {
            const FString ArrivedAt = Notice.DestinationId;
            CancelJoseTakeTheWheel(false, false);
            ShowJoseMessage(FString::Printf(
                TEXT("José Take the Wheel // arrived at %s // holding %.1f m surface standoff"),
                *ArrivedAt,
                JoseArrivalSurfaceRangeMeters));
            return;
        }
        case EEverwardJoseAutopilotStopReason::DestinationUnresolved:
        {
            CancelJoseTakeTheWheel(false, false);
            ShowJoseMessage(TEXT("José stopped: destination geometry is unresolved."), FColor::Orange);
            return;
        }
        case EEverwardJoseAutopilotStopReason::CommandRejected:
        {
            CancelJoseTakeTheWheel(false, false);
            ShowJoseMessage(FString::Printf(TEXT("José autopilot stopped: %s"), *Notice.Detail), FColor::Orange);
            return;
        }
        case EEverwardJoseAutopilotStopReason::None:
        default:
            return;
    }
}

void AEverwardPlayerController::CancelJoseTakeTheWheel(bool bStopVelocity, bool bShowMessage)
{
    const bool bWasEngaged = bJoseAutopilotEngaged;
    bJoseAutopilotEngaged = false;
    JoseDestinationTargetId.Reset();

    if (UProbeSimulationAdapter* Adapter = GetProbeAdapter())
    {
        // Stop the fixed-step governor immediately rather than leaving it
        // engaged on the adapter until a stop/rejection happens to occur.
        Adapter->SetJoseAutopilotGovernorEngaged(false, FString(), 0.0, 0.0, 0.0, 0.0);
        if (bStopVelocity)
        {
            (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        }
    }

    if (bShowMessage && bWasEngaged)
    {
        ShowJoseMessage(TEXT("José released the wheel. Manual control restored."));
    }
}
