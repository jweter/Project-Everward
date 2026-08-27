#include "EverwardProbePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "ProbeSimulationAdapter.h"
#include "UObject/ConstructorHelpers.h"

AEverwardProbePawn::AEverwardProbePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    ProbeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProbePresentation"));
    SetRootComponent(ProbeMesh);
    ProbeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProbeMesh->SetRelativeScale3D(FVector(1.15, 0.55, 0.36));

    // 75 cm matches ProbeStateSnapshot::collision_envelope_radius_m. This
    // component is for presentation/query visibility only; the engine-neutral
    // runtime performs swept contact and resolves authoritative motion.
    ProbeCollisionEnvelope = CreateDefaultSubobject<USphereComponent>(TEXT("ProbeCollisionEnvelope"));
    ProbeCollisionEnvelope->SetupAttachment(ProbeMesh);
    ProbeCollisionEnvelope->SetSphereRadius(75.0f);
    ProbeCollisionEnvelope->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProbeCollisionEnvelope->SetCollisionResponseToAllChannels(ECR_Overlap);
    ProbeCollisionEnvelope->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (SphereMeshAsset.Succeeded())
    {
        ProbeMesh->SetStaticMesh(SphereMeshAsset.Object);
    }

    auto ConfigureOrientationPiece = [this](UStaticMeshComponent* Component)
    {
        Component->SetupAttachment(ProbeMesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
    };

    ForwardSensor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardSensor"));
    ConfigureOrientationPiece(ForwardSensor);
    if (SphereMeshAsset.Succeeded())
    {
        ForwardSensor->SetStaticMesh(SphereMeshAsset.Object);
    }
    ForwardSensor->SetRelativeLocation(FVector(86.0, 0.0, 4.0));
    ForwardSensor->SetRelativeScale3D(FVector(0.34, 0.28, 0.25));

    DorsalMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DorsalMarker"));
    ConfigureOrientationPiece(DorsalMarker);
    if (CubeMeshAsset.Succeeded())
    {
        DorsalMarker->SetStaticMesh(CubeMeshAsset.Object);
    }
    DorsalMarker->SetRelativeLocation(FVector(-8.0, 0.0, 45.0));
    DorsalMarker->SetRelativeScale3D(FVector(0.30, 0.045, 0.16));

    PortShoulder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortShoulder"));
    ConfigureOrientationPiece(PortShoulder);
    if (CubeMeshAsset.Succeeded())
    {
        PortShoulder->SetStaticMesh(CubeMeshAsset.Object);
    }
    PortShoulder->SetRelativeLocation(FVector(-12.0, -48.0, -2.0));
    PortShoulder->SetRelativeScale3D(FVector(0.26, 0.12, 0.07));

    StarboardShoulder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardShoulder"));
    ConfigureOrientationPiece(StarboardShoulder);
    if (CubeMeshAsset.Succeeded())
    {
        StarboardShoulder->SetStaticMesh(CubeMeshAsset.Object);
    }
    StarboardShoulder->SetRelativeLocation(FVector(-12.0, 48.0, -2.0));
    StarboardShoulder->SetRelativeScale3D(FVector(0.26, 0.12, 0.07));

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

void AEverwardProbePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AdvanceCameraAlignedRighting(DeltaSeconds);
}

void AEverwardProbePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (PlayerInputComponent != nullptr)
    {
        PlayerInputComponent->BindKey(
            EKeys::R,
            IE_Pressed,
            this,
            &AEverwardProbePawn::BeginOrCancelCameraAlignedRighting);
    }
}

void AEverwardProbePawn::AdjustCameraZoom(float DeltaCentimeters)
{
    if (CameraBoom == nullptr) return;
    CameraBoom->TargetArmLength = FMath::Clamp(
        CameraBoom->TargetArmLength + DeltaCentimeters,
        MinCameraDistanceCentimeters,
        MaxCameraDistanceCentimeters);
}

void AEverwardProbePawn::BeginOrCancelCameraAlignedRighting()
{
    if (bCameraAlignedRighting)
    {
        bCameraAlignedRighting = false;
        RightingAccumulatorSeconds = 0.0f;
        return;
    }
    if (Controller == nullptr || SimulationAdapter == nullptr) return;

    const FRotator ViewRotation = Controller->GetControlRotation().GetNormalized();
    CameraAlignedRightingTarget = FRotator(ViewRotation.Pitch, ViewRotation.Yaw, 0.0f);
    bCameraAlignedRighting = true;
    RightingAccumulatorSeconds = RightingCommandIntervalSeconds;
}

void AEverwardProbePawn::AdvanceCameraAlignedRighting(float DeltaSeconds)
{
    if (!bCameraAlignedRighting || SimulationAdapter == nullptr) return;

    RightingAccumulatorSeconds += DeltaSeconds;
    if (RightingAccumulatorSeconds < RightingCommandIntervalSeconds) return;
    RightingAccumulatorSeconds = 0.0f;

    const FRotator Current = SimulationAdapter->GetProbeTelemetry().AttitudeDegrees.GetNormalized();
    const float PitchRemaining = FMath::FindDeltaAngleDegrees(Current.Pitch, CameraAlignedRightingTarget.Pitch);
    const float YawRemaining = FMath::FindDeltaAngleDegrees(Current.Yaw, CameraAlignedRightingTarget.Yaw);
    const float RollRemaining = FMath::FindDeltaAngleDegrees(Current.Roll, 0.0f);

    const bool bPitchDone = FMath::Abs(PitchRemaining) <= RightingCompletionToleranceDegrees;
    const bool bYawDone = FMath::Abs(YawRemaining) <= RightingCompletionToleranceDegrees;
    const bool bRollDone = FMath::Abs(RollRemaining) <= RightingCompletionToleranceDegrees;

    if (bPitchDone && bYawDone && bRollDone)
    {
        (void)SimulationAdapter->CommandAdjustAttitudeDegrees(
            FRotator(PitchRemaining, YawRemaining, RollRemaining));
        bCameraAlignedRighting = false;
        return;
    }

    const float MaxStep = RightingDegreesPerSecond * RightingCommandIntervalSeconds;
    const FRotator Step(
        FMath::Clamp(PitchRemaining, -MaxStep, MaxStep),
        FMath::Clamp(YawRemaining, -MaxStep, MaxStep),
        FMath::Clamp(RollRemaining, -MaxStep, MaxStep));

    const FEverwardProbeCommandResult Result = SimulationAdapter->CommandAdjustAttitudeDegrees(Step);
    if (!Result.bAccepted)
    {
        bCameraAlignedRighting = false;
    }
}
