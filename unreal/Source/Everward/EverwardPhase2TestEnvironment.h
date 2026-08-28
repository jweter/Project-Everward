#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EverwardPhase2TestEnvironment.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// Temporary Phase-2 integration environment. The visible scan target is now
// also the first reproducible physical body. Shared meter-space constants keep
// Unreal presentation geometry aligned with the engine-independent contact
// body registered by UProbeSimulationAdapter.
UCLASS()
class EVERWARD_API AEverwardPhase2TestEnvironment : public AActor
{
    GENERATED_BODY()

public:
    AEverwardPhase2TestEnvironment();
    virtual void BeginPlay() override;

    static constexpr const TCHAR* BootstrapScanTargetId = TEXT("phase2-test-target-001");
    static constexpr double BootstrapBodyCenterXMeters = 50.0;
    static constexpr double BootstrapBodyCenterYMeters = 0.0;
    static constexpr double BootstrapBodyCenterZMeters = 0.0;
    static constexpr double BootstrapBodyRadiusMeters = 2.0;

private:
    void ApplyEnvironmentMaterialScaffold();

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<UStaticMeshComponent> ScanTargetMesh;

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<UTextRenderComponent> ScanTargetLabel;

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TObjectPtr<UPointLightComponent> KeyLight;

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2")
    TArray<TObjectPtr<UStaticMeshComponent>> ReferenceMarkers;
};