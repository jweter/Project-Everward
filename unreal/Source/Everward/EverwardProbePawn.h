#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EverwardProbePawn.generated.h"

class UCameraComponent;
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

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ProbeMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Probe", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UProbeSimulationAdapter> SimulationAdapter;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Everward|Camera", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UCameraComponent> ProbeCamera;
};
