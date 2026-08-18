#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkAdapter.generated.h"

class UBenchmarkCaptureSessionComponent;
class UCameraComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class USceneComponent;
class FJsonObject;

UCLASS()
class EVERWARDBENCHMARK_API ABenchmarkAdapter : public AActor
{
    GENERATED_BODY()

public:
    ABenchmarkAdapter();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Everward Benchmark")
    void RestartCanonicalPlayback();

    bool IsCanonicalHandoffReady() const;
    double GetCanonicalDurationSeconds() const;
    FString GetCanonicalScenarioName() const;
    int32 GetCanonicalScenarioVersion() const;

protected:
    virtual void BeginPlay() override;

private:
    static constexpr int32 ExpectedHandoffVersion = 2;
    static constexpr const TCHAR* ExpectedScenarioName = TEXT("icy-asteroid-mining");
    static constexpr double MetersToCentimeters = 100.0;
    static constexpr int32 DebrisParticleCount = 96;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Asteroid;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Probe;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MiningArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Planet;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> DebrisParticles;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UDirectionalLightComponent> StarLight;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UExponentialHeightFogComponent> VolumetricFog;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> BenchmarkCamera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Telemetry;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBenchmarkCaptureSessionComponent> CaptureSession;

    TSharedPtr<FJsonObject> Handoff;
    double ElapsedRealSeconds = 0.0;
    FString ActiveCameraStage;

    bool LoadAndValidateHandoff();
    void ConfigureVisualFeatureShell();
    void PopulateDeterministicDebris();
    void ApplyStaticSceneTruth();
    void ApplyCameraStage(const FString& Stage);
    void ApplyDeterministicAnimation(double ElapsedSeconds);
    void UpdateTelemetry(double ElapsedSeconds);
    FString CameraStageAt(double ElapsedSeconds) const;
    FVector ReadVectorMeters(const TArray<TSharedPtr<class FJsonValue>>& Values) const;
};
