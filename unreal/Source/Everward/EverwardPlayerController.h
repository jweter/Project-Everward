#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EverwardPlayerController.generated.h"

class UProbeSimulationAdapter;

UCLASS()
class EVERWARD_API AEverwardPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void SetupInputComponent() override;

private:
    void ToggleSystemsPanel();
    void SelectNextCapability();
    void SelectPreviousCapability();

    void ExecutePrimarySystemAction();
    void ExecuteSecondarySystemAction();
    void IncreaseSelectedSystemPower();
    void DecreaseSelectedSystemPower();
    void IncreaseForwardVelocity();
    void DecreaseForwardVelocity();
    void StopPropulsion();

    UProbeSimulationAdapter* GetProbeAdapter() const;
    FName GetSelectedCapabilityId() const;
    double GetSelectedCapabilityAllocatedPowerWatts() const;
    void AdjustSelectedSystemPower(double DeltaWatts);
    void AdjustForwardVelocity(double DeltaMetersPerSecond);

    // Temporary Phase-2 target until Phase 3 introduces real world-object
    // targeting. It keeps the command path testable without pretending that
    // target selection already exists.
    UPROPERTY(EditAnywhere, Category="Everward|Phase2")
    FString Phase2ScanTargetId = TEXT("phase2-bootstrap-target");

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double Phase2ScanDurationSeconds = 10.0;

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="1.0"))
    double PowerAdjustmentWatts = 25.0;

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double VelocityAdjustmentMetersPerSecond = 1.0;
};
