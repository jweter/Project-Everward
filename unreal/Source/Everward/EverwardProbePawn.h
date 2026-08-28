#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EverwardProbePawn.generated.h"

class UCameraComponent;
class UInputComponent;
class UProbeSimulationAdapter;
class USceneComponent;
class USphereComponent;
class USpringArmComponent;
class UStaticMeshComponent;

// Prime Generation-1 presentation boundary. The visible body is a modular
// blockout of the canonical Probe A / Scientific Explorer reference family.
// Mechanical/contact truth remains engine-independent; Unreal presents the
// authoritative state and keeps decorative component geometry collision-free.
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
    TObjectPtr<USceneComponent> ProbeRoot;

    // Main load-bearing spine/body. Individual visible systems attach to the
    // root independently so later damage/replacement/articulation can address
    // components rather than treating the probe as one monolithic mesh.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ProbeMesh;

    // Conservative blockout bounding sphere matching the authoritative 8 m
    // collision envelope. Production/compound collision can refine this shape
    // later without allowing decorative meshes to author physical truth.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Physics", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USphereComponent> ProbeCollisionEnvelope;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> CoreHousing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ReactorHousing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> MainEngine;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ForwardSensor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortRadiator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardRadiator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortManeuverPod;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardManeuverPod;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Systems", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> DorsalMarker;

    // Manipulator shoulder/mount points are visible now; articulation arrives
    // in the next slice.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> PortShoulder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe|Manipulators", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> StarboardShoulder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UProbeSimulationAdapter> SimulationAdapter;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UCameraComponent> ProbeCamera;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="500.0"))
    float MinCameraDistanceCentimeters = 1400.0f;

    UPROPERTY(EditAnywhere, Category="Everward|Camera", meta=(ClampMin="500.0"))
    float MaxCameraDistanceCentimeters = 5000.0f;

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