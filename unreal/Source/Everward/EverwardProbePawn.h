#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EverwardProbePawn.generated.h"

class UCameraComponent;
class UInputComponent;
class UProbeSimulationAdapter;
class USpringArmComponent;
class UStaticMeshComponent;

// The first Phase 2 embodiment boundary: one visible presentation that owns
// exactly one Unreal adapter. Mechanical truth remains inside the adapter's
// engine-independent SimulationCore; this pawn is presentation and viewpoint.
UCLASS()
class EVERWARD_API AEverwardProbePawn : public APawn
{
    GENERATED_BODY()

public:
    AEverwardProbePawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category="Everward|Probe")
    UProbeSimulationAdapter* GetSimulationAdapter() const { return SimulationAdapter; }

    UFUNCTION(BlueprintCallable, Category="Everward|Camera")
    void AdjustCameraZoom(float DeltaCentimeters);

private:
    void BeginOrCancelCameraAlignedRighting();
    void AdvanceCameraAlignedRighting(float DeltaSeconds);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ProbeMesh;

    // Temporary Generation-1 orientation skin. These deliberately asymmetric
    // pieces make +X/forward and +Z/up readable during the embodiment test
    // without pretending the final Prime Probe A production mesh exists yet.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ForwardSensor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> DorsalMarker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortShoulder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardShoulder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UProbeSimulationAdapter> SimulationAdapter;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UCameraComponent> ProbeCamera;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="100.0"))
    float MinCameraDistanceCentimeters = 350.0f;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="100.0"))
    float MaxCameraDistanceCentimeters = 1400.0f;

    // Auto-righting is intentionally mechanical rather than instant. The
    // controller/viewpoint supplies the target attitude; every physical
    // attitude change still goes through UProbeSimulationAdapter.
    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="1.0"))
    float RightingDegreesPerSecond = 36.0f;

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.02", ClampMax="0.5"))
    float RightingCommandIntervalSeconds = 0.10f;

    UPROPERTY(EditAnywhere, Category="Everward|Phase2", meta=(ClampMin="0.05"))
    float RightingCompletionToleranceDegrees = 0.5f;

    bool bCameraAlignedRighting = false;
    float RightingAccumulatorSeconds = 0.0f;
    FRotator CameraAlignedRightingTarget = FRotator::ZeroRotator;
};
