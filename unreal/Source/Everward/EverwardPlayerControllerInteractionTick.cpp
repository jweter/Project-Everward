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
        const EEverwardManipulatorArmId ArmId = EverwardHUD != nullptr && EverwardHUD->GetSelectedManipulatorArmIndex() == 1
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

    if (WasInputKeyJustPressed(EKeys::Y))
    {
        ToggleJoseTakeTheWheel();
    }
    if (WasInputKeyJustPressed(EKeys::C))
    {
        ToggleControlledDescent();
    }

    const bool bManualTranslationRequested =
        WasInputKeyJustPressed(EKeys::W) || WasInputKeyJustPressed(EKeys::S) ||
        WasInputKeyJustPressed(EKeys::A) || WasInputKeyJustPressed(EKeys::D) ||
        WasInputKeyJustPressed(EKeys::Q) || WasInputKeyJustPressed(EKeys::E) ||
        WasInputKeyJustPressed(EKeys::Up) || WasInputKeyJustPressed(EKeys::Down);
    if (bManualTranslationRequested)
    {
        if (bJoseAutopilotEngaged)
        {
            CancelJoseTakeTheWheel(false, true);
        }
        if (bControlledDescentEngaged)
        {
            CancelControlledDescent(false, true);
        }
    }
    if (WasInputKeyJustPressed(EKeys::SpaceBar))
    {
        if (bJoseAutopilotEngaged)
        {
            CancelJoseTakeTheWheel(false, true);
        }
        if (bControlledDescentEngaged)
        {
            CancelControlledDescent(false, true);
        }
    }

    AdvanceJoseTakeTheWheel(DeltaSeconds);
    AdvanceControlledDescent(DeltaSeconds);

    if (GEngine != nullptr)
    {
        const FString JoseReadout = bJoseAutopilotEngaged
            ? FString::Printf(TEXT("JOSÉ TAKE THE WHEEL // ENGAGED // %s // [SPACE/WASDQE] TAKE OVER"), *JoseDestinationTargetId)
            : TEXT("JOSÉ TAKE THE WHEEL // [T] SELECT DESTINATION // [Y] ENGAGE");
        GEngine->AddOnScreenDebugMessage(74002, 0.10f, bJoseAutopilotEngaged ? FColor::Cyan : FColor(130, 175, 190), JoseReadout);
        const FString DescentReadout = bControlledDescentEngaged
            ? TEXT("CONTROLLED DESCENT // ENGAGED // [SPACE/WASDQE] TAKE OVER")
            : TEXT("CONTROLLED DESCENT // [C] ENGAGE WHEN PLANETARY BODY IS REGISTERED");
        GEngine->AddOnScreenDebugMessage(74003, 0.10f, bControlledDescentEngaged ? FColor::Cyan : FColor(130, 175, 190), DescentReadout);
    }

    if (Adapter != nullptr && WasInputKeyJustPressed(EKeys::B))
    {
        const FEverwardProbeCommandResult Result = Adapter->CommandEngageTractorField(1000.0);
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(-1, 4.0f, Result.bAccepted ? FColor::Cyan : FColor::Orange,
                Result.bAccepted ? FString::Printf(TEXT("TRACTOR COUPLED // %s"), *Result.Detail)
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
        if (GEngine != nullptr)
        {
            FString TractorReadout = TEXT("TRACTOR // [T] SELECT TARGET // HOLD [B] COUPLE");
            FColor TractorReadoutColor = FColor(130, 175, 190);
            if (Tractor.bHasTarget)
            {
                TractorReadout = FString::Printf(TEXT("TRACTOR %s // %s // %.0f KG / PROBE %.0f KG // %.2fX // %.1f/%.1f M // HOLD [B]"),
                    Tractor.bEngaged ? TEXT("COUPLED") : TEXT("READY"), *Tractor.TargetId,
                    Tractor.TargetMassKilograms, Tractor.ProbeMassKilograms, Tractor.TargetToProbeMassRatio,
                    Tractor.SurfaceRangeMeters, Tractor.MaxSurfaceRangeMeters);
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
                DrawDebugLine(GetWorld(), Probe->GetActorLocation(), TargetPositionCentimeters, FColor::Cyan, false, 0.0f, 0, 10.0f);
                DrawDebugSphere(GetWorld(), TargetPositionCentimeters, 75.0f, 12, FColor::Cyan, false, 0.0f, 0, 6.0f);
            }
        }
    }

    if (WasInputKeyJustPressed(EKeys::Seven)) ToggleSelectedManipulatorTool();
    if (WasInputKeyJustPressed(EKeys::H)) CycleMiningTarget();
    if (WasInputKeyJustPressed(EKeys::P))
    {
        CancelJoseTakeTheWheel(true, false);
        CancelControlledDescent(true, false);
        ToggleAutoApproachMiningTarget();
    }
    if (WasInputKeyJustPressed(EKeys::SpaceBar)) bAutoApproachMiningTarget = false;
    AdvanceAutoApproachMiningTarget(DeltaSeconds);

    if (WasInputKeyJustPressed(EKeys::G) && Adapter != nullptr)
    {
        (void)Adapter->CommandMineBootstrapTarget();
    }
}
