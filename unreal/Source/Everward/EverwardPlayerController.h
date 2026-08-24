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
    virtual void BeginPlay() override;
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
    void IncreaseLateralVelocity();
    void DecreaseLateralVelocity();
    void IncreaseVerticalVelocity();
    void DecreaseVerticalVelocity();
    void StopPropulsion();

    void LookYaw(float Value);
    void LookPitch(float Value);
    void ZoomCamera(float Value);

    UProbeSimulationAdapter* GetProbeAdapter() const;
    FName GetSelectedCapabilityId() const;
    double GetSelectedCapabilityAllocatedPowerWatts() const;
    void AdjustSelectedSystemPower(double DeltaWatts);
    void AdjustVelocityMetersPerSecond(const FVector& DeltaVelocity);

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double Phase2ScanDurationSeconds = 10.0;

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="1.0"))
    double PowerAdjustmentWatts = 25.0;

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.1"))
    double VelocityAdjustmentMetersPerSecond = 1.0;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="0.01"))
    float MouseLookSensitivity = 0.75f;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="1.0"))
    float CameraZoomStepCentimeters = 120.0f;
};
