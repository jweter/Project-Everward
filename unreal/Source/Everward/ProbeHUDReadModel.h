#pragma once

#include "CoreMinimal.h"
#include "ProbeHUDReadModel.generated.h"

struct FEverwardProbeTelemetry;
struct FEverwardTargetSelectionStatus;
struct FEverwardTargetKnowledgeStatus;
struct FEverwardMiningStatus;
struct FEverwardProbeCapability;
struct FEverwardManipulatorArmState;

// Blueprint-readable projection for the compact HUD. This is presentation
// data, never a new simulation or damage-authority model.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardHUDReadModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") bool bHasProbeState = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") FString ProbeId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") float EnergyFraction = 0.0f;
    // There is no authoritative whole-hull integrity value yet. This is the
    // lowest of the four actual subsystem integrities, not fictional hull HP.
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") float WeakestSubsystemFraction = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") float StorageFraction = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") double CoreTemperatureKelvin = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") bool bHasTarget = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") FString TargetId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") double TargetSurfaceRangeMeters = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") FString CurrentAction;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") FString FixItStatus;

    static FEverwardHUDReadModel FromAuthoritativeState(
        const FEverwardProbeTelemetry& Telemetry,
        const FEverwardTargetSelectionStatus& Target,
        const FString& RuntimeFixItStatus);
};

// Presentation-only next-step guidance; never grants a command or predicts that
// mining will succeed. The simulation adapter remains the only authority.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardHUDActionHint
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") FString Text;
    UPROPERTY(BlueprintReadOnly, Category="Everward|HUD") bool bWarning = false;

    static FEverwardHUDActionHint FromAuthoritativeState(
        const FEverwardProbeTelemetry& Telemetry,
        const FEverwardTargetSelectionStatus& Target,
        const FEverwardTargetKnowledgeStatus& Knowledge,
        const FEverwardMiningStatus& Mining,
        const TArray<FEverwardProbeCapability>& Capabilities,
        const TArray<FEverwardManipulatorArmState>& ManipulatorArms);
};
