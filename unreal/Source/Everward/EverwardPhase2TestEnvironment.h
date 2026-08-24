#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EverwardPhase2TestEnvironment.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// Temporary Phase-2 integration environment. It exists so the One Probe
// runtime has reproducible spatial references and a visible scan target even
// before Phase 3 introduces authored star-system content and real targeting.
UCLASS()
class EVERWARD_API AEverwardPhase2TestEnvironment : public AActor
{
    GENERATED_BODY()

public:
    AEverwardPhase2TestEnvironment();

    static constexpr const TCHAR* BootstrapScanTargetId = TEXT("phase2-test-target-001");

private:
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
