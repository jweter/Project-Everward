#pragma once

#include "CoreMinimal.h"
#include "EverwardPhase2TestEnvironment.h"
#include "EverwardZeroGTestEnvironment.generated.h"

class UDirectionalLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// Slice 8 presentation environment: a dedicated free-space test surface that
// reuses the existing authoritative Phase-2 registered-body mechanics while
// removing any dependency on a ground reference. Unreal presentation remains
// read-only with respect to simulation truth.
UCLASS()
class EVERWARD_API AEverwardZeroGTestEnvironment : public AEverwardPhase2TestEnvironment
{
    GENERATED_BODY()

public:
    AEverwardZeroGTestEnvironment();

private:
    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2|ZeroG")
    TObjectPtr<UDirectionalLightComponent> DistantStarLight;

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2|ZeroG")
    TObjectPtr<UStaticMeshComponent> AsteroidReferenceMesh;

    UPROPERTY(VisibleAnywhere, Category="Everward|Phase2|ZeroG")
    TObjectPtr<UTextRenderComponent> AsteroidReferenceLabel;
};
