#include "EverwardProbePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ProbeSimulationAdapter.h"
#include "UObject/ConstructorHelpers.h"

AEverwardProbePawn::AEverwardProbePawn()
{
    PrimaryActorTick.bCanEverTick = false;

    ProbeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProbePresentation"));
    SetRootComponent(ProbeMesh);
    ProbeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProbeMesh->SetRelativeScale3D(FVector(1.0, 0.65, 0.4));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ProbeMeshAsset(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (ProbeMeshAsset.Succeeded())
    {
        ProbeMesh->SetStaticMesh(ProbeMeshAsset.Object);
    }

    // The pawn owns the only simulation adapter in the Phase 2 bootstrap.
    // No other presentation component calls src/simulation directly.
    SimulationAdapter = CreateDefaultSubobject<UProbeSimulationAdapter>(TEXT("ProbeSimulationAdapter"));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ProbeCameraBoom"));
    CameraBoom->SetupAttachment(ProbeMesh);
    CameraBoom->TargetArmLength = 650.0f;
    CameraBoom->SetRelativeRotation(FRotator(-15.0, 0.0, 0.0));
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = false;

    ProbeCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ProbeCamera"));
    ProbeCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    ProbeCamera->bUsePawnControlRotation = false;
}

void AEverwardProbePawn::AdjustCameraZoom(float DeltaCentimeters)
{
    if (CameraBoom == nullptr)
    {
        return;
    }

    CameraBoom->TargetArmLength = FMath::Clamp(
        CameraBoom->TargetArmLength + DeltaCentimeters,
        MinCameraDistanceCentimeters,
        MaxCameraDistanceCentimeters);
}
