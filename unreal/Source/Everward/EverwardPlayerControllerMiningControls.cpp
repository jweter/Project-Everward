#include "EverwardPlayerController.h"

#include "Engine/Engine.h"
#include "EverwardHUD.h"
#include "EverwardPhase2TestEnvironment.h"
#include "EverwardProbePawn.h"
#include "ProbeSimulationAdapter.h"

namespace
{
struct FMiningTargetChoice
{
    FString TargetId;
    FVector CenterMeters = FVector::ZeroVector;
    double RadiusMeters = 0.0;
};

TArray<FMiningTargetChoice> GetAvailableMiningTargets()
{
    // The target-selection interface is intentionally an array from day one.
    // Phase 2 currently exposes one mineable body; later resource bodies can
    // be added here (and then moved behind the scan/resource registry) without
    // changing the player-control contract.
    return {
        FMiningTargetChoice{
            FString(AEverwardPhase2TestEnvironment::BootstrapScanTargetId),
            FVector(
                AEverwardPhase2TestEnvironment::BootstrapBodyCenterXMeters,
                AEverwardPhase2TestEnvironment::BootstrapBodyCenterYMeters,
                AEverwardPhase2TestEnvironment::BootstrapBodyCenterZMeters),
            AEverwardPhase2TestEnvironment::BootstrapBodyRadiusMeters}
    };
}

void ShowMiningControlMessage(const FString& Message)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Message);
    }
}
}

void AEverwardPlayerController::ToggleSelectedManipulatorTool()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    if (Adapter == nullptr || EverwardHUD == nullptr)
    {
        return;
    }

    const EEverwardManipulatorArmId SelectedArm =
        EverwardHUD->GetSelectedManipulatorArmIndex() == 1
            ? EEverwardManipulatorArmId::Starboard
            : EEverwardManipulatorArmId::Port;

    for (const FEverwardManipulatorArmState& ArmState : Adapter->GetManipulatorArmStates())
    {
        if (ArmState.ArmId != SelectedArm)
        {
            continue;
        }

        if (!ArmState.bIsDeployed)
        {
            ShowMiningControlMessage(TEXT("Selected arm must be deployed before attaching a mining tool"));
            return;
        }

        if (ArmState.bToolAttached)
        {
            (void)Adapter->CommandDetachManipulatorTool(SelectedArm);
            ShowMiningControlMessage(SelectedArm == EEverwardManipulatorArmId::Port
                ? TEXT("Mining tool detached from PORT arm")
                : TEXT("Mining tool detached from STARBOARD arm"));
        }
        else
        {
            (void)Adapter->CommandAttachManipulatorTool(SelectedArm);
            ShowMiningControlMessage(SelectedArm == EEverwardManipulatorArmId::Port
                ? TEXT("Mining tool attached to PORT arm")
                : TEXT("Mining tool attached to STARBOARD arm"));
        }
        return;
    }
}

void AEverwardPlayerController::CycleMiningTarget()
{
    const TArray<FMiningTargetChoice> Targets = GetAvailableMiningTargets();
    if (Targets.IsEmpty())
    {
        ShowMiningControlMessage(TEXT("No mining targets available"));
        return;
    }

    SelectedMiningTargetIndex = (SelectedMiningTargetIndex + 1) % Targets.Num();
    bAutoApproachMiningTarget = false;

    const FMiningTargetChoice& Target = Targets[SelectedMiningTargetIndex];
    ShowMiningControlMessage(FString::Printf(TEXT("Mining target selected: %s"), *Target.TargetId));
}

void AEverwardPlayerController::ToggleAutoApproachMiningTarget()
{
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (Adapter == nullptr)
    {
        return;
    }

    const FEverwardMiningStatus MiningStatus = Adapter->GetMiningStatus();
    if (!MiningStatus.bSurveyed)
    {
        ShowMiningControlMessage(TEXT("Scan and survey the mining target before auto-approach"));
        return;
    }

    bAutoApproachMiningTarget = !bAutoApproachMiningTarget;
    if (!bAutoApproachMiningTarget)
    {
        (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        ShowMiningControlMessage(TEXT("Mining auto-approach cancelled"));
        return;
    }

    ShowMiningControlMessage(TEXT("Mining auto-approach engaged; SPACE cancels/holds position"));
}

void AEverwardPlayerController::AdvanceAutoApproachMiningTarget(float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (!bAutoApproachMiningTarget)
    {
        return;
    }

    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(GetPawn());
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    if (Adapter == nullptr || Probe == nullptr || EverwardHUD == nullptr)
    {
        bAutoApproachMiningTarget = false;
        return;
    }

    const TArray<FMiningTargetChoice> Targets = GetAvailableMiningTargets();
    if (Targets.IsEmpty())
    {
        bAutoApproachMiningTarget = false;
        return;
    }

    SelectedMiningTargetIndex = FMath::Clamp(SelectedMiningTargetIndex, 0, Targets.Num() - 1);
    const FMiningTargetChoice& Target = Targets[SelectedMiningTargetIndex];

    // Stage the probe beside the target rather than nose-first. That keeps the
    // long Prime hull clear while placing the selected side-mounted arm toward
    // the resource body. Port arm => target on probe's port side; starboard
    // arm => target on starboard side.
    const bool bStarboardArm = EverwardHUD->GetSelectedManipulatorArmIndex() == 1;
    const double SideSign = bStarboardArm ? -1.0 : 1.0;
    const FVector StagingPointMeters = Target.CenterMeters + FVector(
        0.0,
        SideSign * (Target.RadiusMeters + AutoApproachSideStandoffMeters),
        0.0);

    const FVector ProbePositionMeters = Probe->GetActorLocation() * 0.01;
    const FVector DeltaMeters = StagingPointMeters - ProbePositionMeters;
    const double DistanceMeters = DeltaMeters.Length();

    if (DistanceMeters <= AutoApproachStopToleranceMeters)
    {
        (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
        bAutoApproachMiningTarget = false;
        ShowMiningControlMessage(TEXT("Mining staging position reached; articulate selected arm and mine with G"));
        return;
    }

    const double Speed = FMath::Min(AutoApproachSpeedMetersPerSecond, FMath::Max(0.25, DistanceMeters));
    const FVector CommandVelocity = DeltaMeters.GetSafeNormal() * Speed;
    const FEverwardProbeCommandResult Result = Adapter->CommandSetVelocityMetersPerSecond(CommandVelocity);
    if (!Result.bAccepted)
    {
        bAutoApproachMiningTarget = false;
        ShowMiningControlMessage(FString::Printf(TEXT("Auto-approach stopped: %s"), *Result.Detail));
    }
}
