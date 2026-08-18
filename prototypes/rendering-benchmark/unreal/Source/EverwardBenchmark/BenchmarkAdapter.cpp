#include "BenchmarkAdapter.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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

    StarLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StarLight"));
    StarLight->SetupAttachment(SceneRoot);

    BenchmarkCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BenchmarkCamera"));
    BenchmarkCamera->SetupAttachment(SceneRoot);

    Telemetry = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Telemetry"));
    Telemetry->SetupAttachment(SceneRoot);
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
}

void ABenchmarkAdapter::UpdateTelemetry(double ElapsedSeconds)
{
    const double SimulatedSeconds = ElapsedSeconds * Handoff->GetNumberField(TEXT("simulation_seconds_per_real_second"));
    Telemetry->SetText(FText::FromString(FString::Printf(
        TEXT("EVERWARD // UNREAL PROTOTYPE C\ncamera: %s\nreal: %.3f s   simulated: %.3f s"),
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
