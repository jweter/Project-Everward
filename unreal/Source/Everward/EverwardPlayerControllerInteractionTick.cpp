#include "EverwardPlayerController.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "EverwardHUD.h"
#include "EverwardProbePawn.h"
#include "InputCoreTypes.h"
#include "ProbeSimulationAdapter.h"

void AEverwardPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(GetPawn());
    AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Probe != nullptr)
    {
        const bool bHighlightEnabled = EverwardHUD != nullptr && EverwardHUD->IsManipulatorPanelExpanded();
        const EEverwardManipulatorArmId ArmId = EverwardHUD != nullptr &&
            EverwardHUD->GetSelectedManipulatorArmIndex() == 1
                ? EEverwardManipulatorArmId::Starboard
                : EEverwardManipulatorArmId::Port;

        EEverwardManipulatorJoint Joint = EEverwardManipulatorJoint::Shoulder;
        if (EverwardHUD != nullptr)
        {
            switch (EverwardHUD->GetSelectedManipulatorJointIndex())
            {
                case 1: Joint = EEverwardManipulatorJoint::Elbow; break;
                case 2: Joint = EEverwardManipulatorJoint::Wrist; break;
                default: Joint = EEverwardManipulatorJoint::Shoulder; break;
            }
        }
        Probe->SetManipulatorSelectionHighlight(bHighlightEnabled, ArmId, Joint);
    }

    // "José Take the Wheel" is the friendly automatic-driving layer. It
    // deliberately composes with the existing physical target selector:
    //   T = cycle/select destination
    //   Y = give José the wheel / cancel
    //   SPACE or any manual translation trim = immediate manual takeover
    // The first playable version uses safe surface-range arrival and progressive
    // deceleration. Rich orbital route planning can evolve behind the same UI.
    if (WasInputKeyJustPressed(EKeys::Y))
    {
        ToggleJoseTakeTheWheel();
    }
    // Controlled descent is a velocity-governor copilot rather than a
    // destination autopilot, so it composes with José/manual flight the same
    // way mining auto-approach already does: engaging one releases the
    // others, and any manual override below releases this too.
    //   C = engage/cancel controlled descent near a registered planetary body
    if (WasInputKeyJustPressed(EKeys::C))
    {
        ToggleControlledDescent();
    }
    // Controlled hover is the same velocity-governor family as controlled
    // descent (it holds altitude rather than tapering an approach), so it
    // shares the exact composition/manual-override rules:
    //   V = engage/cancel controlled hover near a registered planetary body
    if (WasInputKeyJustPressed(EKeys::V))
    {
        ToggleControlledHover();
    }

    const bool bManualTranslationRequested =
        WasInputKeyJustPressed(EKeys::W) ||
        WasInputKeyJustPressed(EKeys::S) ||
        WasInputKeyJustPressed(EKeys::A) ||
        WasInputKeyJustPressed(EKeys::D) ||
        WasInputKeyJustPressed(EKeys::Q) ||
        WasInputKeyJustPressed(EKeys::E) ||
        WasInputKeyJustPressed(EKeys::Up) ||
        WasInputKeyJustPressed(EKeys::Down);
    if (bManualTranslationRequested && bJoseAutopilotEngaged)
    {
        // Input bindings have already applied the player's requested trim by
        // Tick time; cancel without zeroing so the manual command wins.
        CancelJoseTakeTheWheel(false, true);
    }
    if (bManualTranslationRequested && bControlledDescentEngaged)
    {
        CancelControlledDescent(false, true);
    }
    if (bManualTranslationRequested && bControlledHoverEngaged)
    {
        CancelControlledHover(false, true);
    }
    if (WasInputKeyJustPressed(EKeys::SpaceBar) && bJoseAutopilotEngaged)
    {
        CancelJoseTakeTheWheel(false, true);
    }
    if (WasInputKeyJustPressed(EKeys::SpaceBar) && bControlledDescentEngaged)
    {
        CancelControlledDescent(false, true);
    }
    if (WasInputKeyJustPressed(EKeys::SpaceBar) && bControlledHoverEngaged)
    {
        CancelControlledHover(false, true);
    }

    AdvanceJoseTakeTheWheel(DeltaSeconds);
    AdvanceControlledDescent(DeltaSeconds);
    AdvanceControlledHover(DeltaSeconds);

    if (GEngine != nullptr)
    {
        const FString JoseReadout = bJoseAutopilotEngaged
            ? FString::Printf(
                TEXT("JOSÉ TAKE THE WHEEL // ENGAGED // %s // [SPACE/WASDQE] TAKE OVER"),
                *JoseDestinationTargetId)
            : TEXT("JOSÉ TAKE THE WHEEL // [T] SELECT DESTINATION // [Y] ENGAGE");
        GEngine->AddOnScreenDebugMessage(
            74002,
            0.10f,
            bJoseAutopilotEngaged ? FColor::Cyan : FColor(130, 175, 190),
            JoseReadout);

        const FString DescentReadout = bControlledDescentEngaged
            ? TEXT("CONTROLLED DESCENT // ENGAGED // [SPACE/WASDQE] TAKE OVER")
            : TEXT("CONTROLLED DESCENT // [C] ENGAGE NEAR A REGISTERED PLANETARY BODY");
        GEngine->AddOnScreenDebugMessage(
            74003,
            0.10f,
            bControlledDescentEngaged ? FColor::Cyan : FColor(130, 175, 190),
            DescentReadout);

        const FString HoverReadout = bControlledHoverEngaged
            ? TEXT("CONTROLLED HOVER // ENGAGED // [SPACE/WASDQE] TAKE OVER")
            : TEXT("CONTROLLED HOVER // [V] ENGAGE NEAR A REGISTERED PLANETARY BODY");
        GEngine->AddOnScreenDebugMessage(
            74004,
            0.10f,
            bControlledHoverEngaged ? FColor::Cyan : FColor(130, 175, 190),
            HoverReadout);
    }

    // First playable tractor-field control. T already owns physical target
    // selection, so tractor use composes with that existing interaction rather
    // than inventing another target list:
    //   T = cycle/select physical target
    //   hold B = couple tractor field at 1 kN
    //   release B = disengage; the target keeps acquired zero-g drift
    // The adapter fixed-steps tractor physics using tractor_field.hpp; this
    // controller only owns input and presentation.
    if (Adapter != nullptr && WasInputKeyJustPressed(EKeys::B))
    {
        const FEverwardProbeCommandResult Result = Adapter->CommandEngageTractorField(1000.0);
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                4.0f,
                Result.bAccepted ? FColor::Cyan : FColor::Orange,
                Result.bAccepted
                    ? FString::Printf(TEXT("TRACTOR COUPLED // %s"), *Result.Detail)
                    : FString::Printf(TEXT("TRACTOR REJECTED // %s"), *Result.Detail));
        }
    }
    if (Adapter != nullptr && WasInputKeyJustReleased(EKeys::B))
    {
        (void)Adapter->CommandDisengageTractorField();
    }

    if (Adapter != nullptr)
    {
        Adapter->AdvanceTractorField(DeltaSeconds);

        const FEverwardTractorFieldStatus Tractor = Adapter->GetTractorFieldStatus();

        // Temporary Product Reality readout until the dedicated tool HUD is
        // built. A fixed message key updates one line in place every frame,
        // so the new control is discoverable rather than hidden in docs.
        if (GEngine != nullptr)
        {
            FString TractorReadout = TEXT("TRACTOR // [T] SELECT TARGET // HOLD [B] COUPLE");
            FColor TractorReadoutColor = FColor(130, 175, 190);
            if (Tractor.bHasTarget)
            {
                TractorReadout = FString::Printf(
                    TEXT("TRACTOR %s // %s // %.0f KG / PROBE %.0f KG // %.2fX // %.1f/%.1f M // HOLD [B]"),
                    Tractor.bEngaged ? TEXT("COUPLED") : TEXT("READY"),
                    *Tractor.TargetId,
                    Tractor.TargetMassKilograms,
                    Tractor.ProbeMassKilograms,
                    Tractor.TargetToProbeMassRatio,
                    Tractor.SurfaceRangeMeters,
                    Tractor.MaxSurfaceRangeMeters);
                TractorReadoutColor = Tractor.bEngaged ? FColor::Cyan : FColor(150, 215, 230);
            }
            GEngine->AddOnScreenDebugMessage(74001, 0.10f, TractorReadoutColor, TractorReadout);
        }

        if (Tractor.bEngaged && Tractor.bHasTarget && Probe != nullptr && GetWorld() != nullptr)
        {
            FVector TargetPositionMeters;
            if (Adapter->GetStaticBodyPositionMeters(Tractor.TargetId, TargetPositionMeters))
            {
                const FVector TargetPositionCentimeters = TargetPositionMeters * 100.0;
                DrawDebugLine(
                    GetWorld(),
                    Probe->GetActorLocation(),
                    TargetPositionCentimeters,
                    FColor::Cyan,
                    false,
                    0.0f,
                    0,
                    10.0f);
                DrawDebugSphere(
                    GetWorld(),
                    TargetPositionCentimeters,
                    75.0f,
                    12,
                    FColor::Cyan,
                    false,
                    0.0f,
                    0,
                    6.0f);
            }
        }
    }

    // Mining control surface:
    //   N = selected arm (existing HUD control)
    //   7 = attach/detach tool on that selected arm
    //   H = cycle available mining targets
    //   P = auto-approach the selected surveyed target to an arm-side staging point
    //   G = attempt extraction
    if (WasInputKeyJustPressed(EKeys::Seven))
    {
        ToggleSelectedManipulatorTool();
    }
    if (WasInputKeyJustPressed(EKeys::H))
    {
        CycleMiningTarget();
    }
    if (WasInputKeyJustPressed(EKeys::P))
    {
        CancelJoseTakeTheWheel(true, false);
        CancelControlledDescent(true, false);
        CancelControlledHover(true, false);
        ToggleAutoApproachMiningTarget();
    }
    if (WasInputKeyJustPressed(EKeys::SpaceBar))
    {
        bAutoApproachMiningTarget = false;
    }

    AdvanceAutoApproachMiningTarget(DeltaSeconds);

    // First mining interaction. G is intentionally global during this early
    // physical-work slice: the player should be able to position the probe,
    // articulate the arm, and attempt extraction without navigating away from
    // the manipulator view. The command itself enforces scan/tool/reach/storage
    // requirements authoritatively and reports why an attempt fails.
    if (WasInputKeyJustPressed(EKeys::G))
    {
        if (Adapter != nullptr)
        {
            (void)Adapter->CommandMineBootstrapTarget();
        }
    }
}
