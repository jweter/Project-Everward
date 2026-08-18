#include "BenchmarkAdapter.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

ABenchmarkAdapter::ABenchmarkAdapter()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Asteroid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Asteroid"));
    Asteroid->SetupAttachment(SceneRoot);

    Probe = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Probe"));
    Probe->SetupAttachment(SceneRoot);

    MiningArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiningArm"));
    MiningArm->SetupAttachment(Probe);

    Planet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Planet"));
    Planet->SetupAttachment(SceneRoot);

    DebrisParticles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DebrisParticles"));
    DebrisParticles->SetupAttachment(SceneRoot);

    StarLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StarLight"));
    StarLight->SetupAttachment(SceneRoot);

    VolumetricFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("VolumetricFog"));
    VolumetricFog->SetupAttachment(SceneRoot);

    BenchmarkCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BenchmarkCamera"));
    BenchmarkCamera->SetupAttachment(SceneRoot);

    Telemetry = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Telemetry"));
    Telemetry->SetupAttachment(BenchmarkCamera);

    ConfigureVisualFeatureShell();
}

void ABenchmarkAdapter::BeginPlay()
{
    Super::BeginPlay();

    if (!LoadAndValidateHandoff())
    {
        SetActorTickEnabled(false);
        return;
    }

    ApplyStaticSceneTruth();
    PopulateDeterministicDebris();
    RestartCanonicalPlayback();
}

void ABenchmarkAdapter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!Handoff.IsValid())
    {
        return;
    }

    const double DurationSeconds = Handoff->GetNumberField(TEXT("duration_seconds"));
    ElapsedRealSeconds = FMath::Fmod(ElapsedRealSeconds + static_cast<double>(DeltaSeconds), DurationSeconds);

    const FString Stage = CameraStageAt(ElapsedRealSeconds);
    if (Stage != ActiveCameraStage)
    {
        ApplyCameraStage(Stage);
    }

    ApplyDeterministicAnimation(ElapsedRealSeconds);
    UpdateTelemetry(ElapsedRealSeconds);
}

void ABenchmarkAdapter::RestartCanonicalPlayback()
{
    if (!Handoff.IsValid())
    {
        return;
    }

    ElapsedRealSeconds = 0.0;
    ApplyCameraStage(CameraStageAt(0.0));
    ApplyDeterministicAnimation(0.0);
    UpdateTelemetry(0.0);
}

bool ABenchmarkAdapter::LoadAndValidateHandoff()
{
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("benchmark_handoff.json"));
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *Path))
    {
        UE_LOG(LogTemp, Error, TEXT("Missing canonical benchmark handoff: %s"), *Path);
        return false;
    }

    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Handoff) || !Handoff.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Benchmark handoff must contain a JSON object"));
        return false;
    }

    const TCHAR* RequiredKeys[] = {
        TEXT("handoff_version"), TEXT("scenario_name"), TEXT("scenario_version"),
        TEXT("duration_seconds"), TEXT("target_resolution"), TEXT("target_fps"),
        TEXT("simulation_seconds_per_real_second"), TEXT("camera_sequence"),
        TEXT("camera_stage_durations_seconds"), TEXT("required_scene_features"),
        TEXT("objects"), TEXT("cameras"), TEXT("animation_periods_seconds"),
        TEXT("feature_bindings")
    };

    for (const TCHAR* Key : RequiredKeys)
    {
        if (!Handoff->HasField(Key))
        {
            UE_LOG(LogTemp, Error, TEXT("Benchmark handoff missing required key: %s"), Key);
            return false;
        }
    }

    if (Handoff->GetIntegerField(TEXT("handoff_version")) != ExpectedHandoffVersion)
    {
        UE_LOG(LogTemp, Error, TEXT("Unsupported benchmark handoff version"));
        return false;
    }

    if (Handoff->GetStringField(TEXT("scenario_name")) != ExpectedScenarioName)
    {
        UE_LOG(LogTemp, Error, TEXT("Unexpected benchmark scenario"));
        return false;
    }

    return true;
}

void ABenchmarkAdapter::ConfigureVisualFeatureShell()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    if (SphereMesh.Succeeded())
    {
        Asteroid->SetStaticMesh(SphereMesh.Object);
        Planet->SetStaticMesh(SphereMesh.Object);
        DebrisParticles->SetStaticMesh(SphereMesh.Object);
    }
    if (CubeMesh.Succeeded())
    {
        Probe->SetStaticMesh(CubeMesh.Object);
    }
    if (CylinderMesh.Succeeded())
    {
        MiningArm->SetStaticMesh(CylinderMesh.Object);
    }

    // Renderer-owned presentation only. Canonical positions and timing still come from the handoff.
    Asteroid->SetWorldScale3D(FVector(8.5, 7.2, 6.8));
    Probe->SetWorldScale3D(FVector(0.35, 0.55, 0.25));
    MiningArm->SetWorldScale3D(FVector(0.12, 0.12, 0.8));
    Planet->SetWorldScale3D(FVector(180.0));
    DebrisParticles->SetWorldScale3D(FVector(0.025));

    StarLight->SetIntensity(8.0f);
    StarLight->SetLightColor(FLinearColor(1.0f, 0.94f, 0.84f));
    StarLight->SetCastShadows(true);

    VolumetricFog->SetFogDensity(0.0025f);
    VolumetricFog->SetFogHeightFalloff(0.08f);
    VolumetricFog->SetVolumetricFog(true);
    VolumetricFog->SetVolumetricFogExtinctionScale(0.35f);

    BenchmarkCamera->bConstrainAspectRatio = true;
    BenchmarkCamera->AspectRatio = 2560.0f / 1440.0f;

    Telemetry->SetRelativeLocation(FVector(180.0, -78.0, -42.0));
    Telemetry->SetRelativeRotation(FRotator(0.0, 180.0, 0.0));
    Telemetry->SetWorldSize(18.0f);
    Telemetry->SetHorizontalAlignment(EHTA_Left);
}

void ABenchmarkAdapter::PopulateDeterministicDebris()
{
    DebrisParticles->ClearInstances();

    // Fixed low-discrepancy placement gives both repeatability and a particle-like field without RNG.
    const FVector AsteroidCenter = Asteroid->GetComponentLocation();
    for (int32 Index = 0; Index < DebrisParticleCount; ++Index)
    {
        const double T = static_cast<double>(Index) / static_cast<double>(DebrisParticleCount);
        const double Angle = T * UE_TWO_PI * 7.0;
        const double RadiusM = 9.0 + static_cast<double>((Index * 37) % 29) * 0.18;
        const double HeightM = -2.0 + static_cast<double>((Index * 19) % 23) * 0.18;
        const FVector Offset(
            FMath::Cos(Angle) * RadiusM * MetersToCentimeters,
            FMath::Sin(Angle) * RadiusM * MetersToCentimeters,
            HeightM * MetersToCentimeters);
        const FTransform InstanceTransform(FRotator::ZeroRotator, AsteroidCenter + Offset, FVector(1.0));
        DebrisParticles->AddInstance(InstanceTransform, true);
    }
}

void ABenchmarkAdapter::ApplyStaticSceneTruth()
{
    const TSharedPtr<FJsonObject> Objects = Handoff->GetObjectField(TEXT("objects"));
    Asteroid->SetWorldLocation(ReadVectorMeters(Objects->GetObjectField(TEXT("asteroid"))->GetArrayField(TEXT("position_m"))));
    Probe->SetWorldLocation(ReadVectorMeters(Objects->GetObjectField(TEXT("probe"))->GetArrayField(TEXT("position_m"))));
    Planet->SetWorldLocation(ReadVectorMeters(Objects->GetObjectField(TEXT("planet"))->GetArrayField(TEXT("position_m"))));

    const FVector Direction = ReadVectorMeters(Objects->GetObjectField(TEXT("star_light"))->GetArrayField(TEXT("direction"))).GetSafeNormal();
    StarLight->SetWorldRotation(Direction.Rotation());
}

FString ABenchmarkAdapter::CameraStageAt(double ElapsedSeconds) const
{
    const TArray<TSharedPtr<FJsonValue>>& Sequence = Handoff->GetArrayField(TEXT("camera_sequence"));
    const TArray<TSharedPtr<FJsonValue>>& Durations = Handoff->GetArrayField(TEXT("camera_stage_durations_seconds"));

    double Boundary = 0.0;
    for (int32 Index = 0; Index < Sequence.Num(); ++Index)
    {
        Boundary += Durations[Index]->AsNumber();
        if (ElapsedSeconds < Boundary)
        {
            return Sequence[Index]->AsString();
        }
    }

    return Sequence.Last()->AsString();
}

void ABenchmarkAdapter::ApplyCameraStage(const FString& Stage)
{
    const TSharedPtr<FJsonObject> Cameras = Handoff->GetObjectField(TEXT("cameras"));
    const TSharedPtr<FJsonObject> Config = Cameras->GetObjectField(Stage);

    const FVector Position = ReadVectorMeters(Config->GetArrayField(TEXT("position_m")));
    const FVector Target = ReadVectorMeters(Config->GetArrayField(TEXT("target_m")));
    BenchmarkCamera->SetWorldLocation(Position);
    BenchmarkCamera->SetWorldRotation((Target - Position).Rotation());
    BenchmarkCamera->SetFieldOfView(Config->GetNumberField(TEXT("vertical_fov_degrees")));
    ActiveCameraStage = Stage;
}

void ABenchmarkAdapter::ApplyDeterministicAnimation(double ElapsedSeconds)
{
    const TSharedPtr<FJsonObject> Periods = Handoff->GetObjectField(TEXT("animation_periods_seconds"));
    const double AsteroidPeriod = Periods->GetNumberField(TEXT("asteroid_rotation"));
    const double MiningPeriod = Periods->GetNumberField(TEXT("mining_mechanism"));

    const double AsteroidPhase = FMath::Fmod(ElapsedSeconds, AsteroidPeriod) / AsteroidPeriod;
    Asteroid->SetWorldRotation(FRotator(0.0, AsteroidPhase * 360.0, 0.0));

    const double MiningPhase = FMath::Fmod(ElapsedSeconds, MiningPeriod) / MiningPeriod;
    const double Wave = FMath::Sin(MiningPhase * UE_TWO_PI);
    MiningArm->SetRelativeRotation(FRotator(Wave * 24.0, 0.0, 0.0));
    MiningArm->SetRelativeLocation(FVector(0.0, 0.0, (17.0 + Wave * 3.0) * MetersToCentimeters));

    // Debris motion is deterministic presentation derived from canonical playback time, never RNG.
    const FRotator DebrisRotation(0.0, FMath::Fmod(ElapsedSeconds * 3.0, 360.0), 0.0);
    DebrisParticles->SetWorldRotation(DebrisRotation);
}

void ABenchmarkAdapter::UpdateTelemetry(double ElapsedSeconds)
{
    const double SimulatedSeconds = ElapsedSeconds * Handoff->GetNumberField(TEXT("simulation_seconds_per_real_second"));
    Telemetry->SetText(FText::FromString(FString::Printf(
        TEXT("EVERWARD // UNREAL PROTOTYPE C\ncamera: %s\nreal: %.3f s   simulated: %.3f s\nrender: stellar light / volumetric fog / deterministic debris"),
        *ActiveCameraStage,
        ElapsedSeconds,
        SimulatedSeconds)));
}

FVector ABenchmarkAdapter::ReadVectorMeters(const TArray<TSharedPtr<FJsonValue>>& Values) const
{
    check(Values.Num() == 3);
    return FVector(
        Values[0]->AsNumber() * MetersToCentimeters,
        Values[1]->AsNumber() * MetersToCentimeters,
        Values[2]->AsNumber() * MetersToCentimeters);
}
