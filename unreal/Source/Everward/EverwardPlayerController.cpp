#include "EverwardPlayerController.h"

#include "EverwardHUD.h"
#include "EverwardProbePawn.h"
#include "InputCoreTypes.h"
#include "ProbeSimulationAdapter.h"

namespace
{
bool ResolvePowerSubsystem(FName CapabilityId, EEverwardPowerSubsystem& OutSubsystem)
{
    if (CapabilityId == FName(TEXT("sensors")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Sensors;
        return true;
    }
    if (CapabilityId == FName(TEXT("propulsion")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Propulsion;
        return true;
    }
    if (CapabilityId == FName(TEXT("computation")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Computation;
        return true;
    }
    if (CapabilityId == FName(TEXT("thermal")))
    {
        OutSubsystem = EEverwardPowerSubsystem::Thermal;
        return true;
    }
    return false;
}
}

void AEverwardPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent == nullptr)
    {
        return;
    }

    InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AEverwardPlayerController::ToggleSystemsPanel);
    InputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AEverwardPlayerController::SelectNextCapability);
    InputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AEverwardPlayerController::SelectPreviousCapability);

    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AEverwardPlayerController::ExecutePrimarySystemAction);
    InputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this, &AEverwardPlayerController::ExecuteSecondarySystemAction);
    InputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &AEverwardPlayerController::IncreaseSelectedSystemPower);
    InputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &AEverwardPlayerController::DecreaseSelectedSystemPower);
    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AEverwardPlayerController::IncreaseForwardVelocity);
    InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AEverwardPlayerController::DecreaseForwardVelocity);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AEverwardPlayerController::StopPropulsion);
}

void AEverwardPlayerController::ToggleSystemsPanel()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->ToggleSystemsPanel();
    }
}

void AEverwardPlayerController::SelectNextCapability()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->SelectNextCapability();
    }
}

void AEverwardPlayerController::SelectPreviousCapability()
{
    if (AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD()))
    {
        EverwardHUD->SelectPreviousCapability();
    }
}

void AEverwardPlayerController::ExecutePrimarySystemAction()
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    const FName CapabilityId = GetSelectedCapabilityId();
    if (CapabilityId == FName(TEXT("sensors")))
    {
        (void)Adapter->CommandStartScan(Phase2ScanTargetId, Phase2ScanDurationSeconds);
    }
    else if (CapabilityId == FName(TEXT("computation")))
    {
        (void)Adapter->CommandInstallBasicSurvivalPolicy();
    }
}

void AEverwardPlayerController::ExecuteSecondarySystemAction()
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    const FName CapabilityId = GetSelectedCapabilityId();
    if (CapabilityId == FName(TEXT("sensors")))
    {
        (void)Adapter->CommandCancelScan();
    }
    else if (CapabilityId == FName(TEXT("computation")))
    {
        (void)Adapter->CommandClearSoftwarePolicy();
    }
}

void AEverwardPlayerController::IncreaseSelectedSystemPower()
{
    AdjustSelectedSystemPower(PowerAdjustmentWatts);
}

void AEverwardPlayerController::DecreaseSelectedSystemPower()
{
    AdjustSelectedSystemPower(-PowerAdjustmentWatts);
}

void AEverwardPlayerController::IncreaseForwardVelocity()
{
    AdjustForwardVelocity(VelocityAdjustmentMetersPerSecond);
}

void AEverwardPlayerController::DecreaseForwardVelocity()
{
    AdjustForwardVelocity(-VelocityAdjustmentMetersPerSecond);
}

void AEverwardPlayerController::StopPropulsion()
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    if (GetSelectedCapabilityId() == FName(TEXT("propulsion")))
    {
        (void)Adapter->CommandSetVelocityMetersPerSecond(FVector::ZeroVector);
    }
}

UProbeSimulationAdapter* AEverwardPlayerController::GetProbeAdapter() const
{
    const AEverwardProbePawn* Probe = Cast<AEverwardProbePawn>(GetPawn());
    return Probe != nullptr ? Probe->GetSimulationAdapter() : nullptr;
}

FName AEverwardPlayerController::GetSelectedCapabilityId() const
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || Adapter == nullptr)
    {
        return NAME_None;
    }

    const TArray<FEverwardProbeCapability> Capabilities = Adapter->GetInstalledCapabilities();
    if (Capabilities.IsEmpty())
    {
        return NAME_None;
    }

    const int32 Index = FMath::Clamp(EverwardHUD->GetSelectedCapabilityIndex(), 0, Capabilities.Num() - 1);
    return Capabilities[Index].CapabilityId;
}

double AEverwardPlayerController::GetSelectedCapabilityAllocatedPowerWatts() const
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || Adapter == nullptr)
    {
        return 0.0;
    }

    const TArray<FEverwardProbeCapability> Capabilities = Adapter->GetInstalledCapabilities();
    if (Capabilities.IsEmpty())
    {
        return 0.0;
    }

    const int32 Index = FMath::Clamp(EverwardHUD->GetSelectedCapabilityIndex(), 0, Capabilities.Num() - 1);
    return Capabilities[Index].AllocatedPowerWatts;
}

void AEverwardPlayerController::AdjustSelectedSystemPower(double DeltaWatts)
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    EEverwardPowerSubsystem Subsystem = EEverwardPowerSubsystem::Sensors;
    if (!ResolvePowerSubsystem(GetSelectedCapabilityId(), Subsystem))
    {
        return;
    }

    const double RequestedWatts = FMath::Max(0.0, GetSelectedCapabilityAllocatedPowerWatts() + DeltaWatts);
    (void)Adapter->CommandAllocatePower(Subsystem, RequestedWatts);
}

void AEverwardPlayerController::AdjustForwardVelocity(double DeltaMetersPerSecond)
{
    const AEverwardHUD* EverwardHUD = Cast<AEverwardHUD>(GetHUD());
    UProbeSimulationAdapter* Adapter = GetProbeAdapter();
    if (EverwardHUD == nullptr || !EverwardHUD->IsSystemsPanelExpanded() || Adapter == nullptr)
    {
        return;
    }

    if (GetSelectedCapabilityId() != FName(TEXT("propulsion")))
    {
        return;
    }

    FVector RequestedVelocity = Adapter->GetProbeTelemetry().VelocityMetersPerSecond;
    RequestedVelocity.X += DeltaMetersPerSecond;
    (void)Adapter->CommandSetVelocityMetersPerSecond(RequestedVelocity);
}
