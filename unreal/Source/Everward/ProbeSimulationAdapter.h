#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProbeSimulationAdapter.generated.h"

namespace everward::simulation
{
class SimulationCore;
}

UENUM(BlueprintType)
enum class EEverwardPowerSubsystem : uint8
{
    Sensors UMETA(DisplayName="Sensors"),
    Propulsion UMETA(DisplayName="Propulsion"),
    Computation UMETA(DisplayName="Computation"),
    Thermal UMETA(DisplayName="Thermal")
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardProbeCommandResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Command")
    int64 Sequence = 0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Command")
    FName CommandId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Command")
    bool bAccepted = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Command")
    FString Detail;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardProbeTelemetry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    FString ProbeId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    int32 Generation = 0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    int64 SimulationTick = 0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double SimulationTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double MassKilograms = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double StoredEnergyJoules = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double EnergyCapacityJoules = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double EnergyGenerationWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double PowerCapacityWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double PowerAllocatedSensorsWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double PowerAllocatedPropulsionWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double PowerAllocatedComputationWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double PowerAllocatedThermalWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double TotalPowerAllocatedWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double TemperatureKelvin = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double StorageUsedKilograms = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double StorageCapacityKilograms = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    FVector VelocityMetersPerSecond = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    bool bIsScanning = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    FString ActiveScanTargetId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double ScanRemainingSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    bool bIsOverheated = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    bool bIsEnergyDepleted = false;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardProbeCapability
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    FName CapabilityId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    FString Description;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    bool bInstalled = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    bool bOperational = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    bool bAvailable = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    bool bSupportsManualControl = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    bool bSupportsAutomation = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    double AllocatedPowerWatts = 0.0;
};

UCLASS(ClassGroup=(Everward), meta=(BlueprintSpawnableComponent))
class EVERWARD_API UProbeSimulationAdapter : public UActorComponent
{
    GENERATED_BODY()

public:
    UProbeSimulationAdapter();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintPure, Category="Everward|Simulation")
    int64 GetSimulationTick() const;

    UFUNCTION(BlueprintPure, Category="Everward|Simulation")
    FVector GetProbePositionMeters() const;

    UFUNCTION(BlueprintPure, Category="Everward|Simulation")
    FEverwardProbeTelemetry GetProbeTelemetry() const;

    UFUNCTION(BlueprintPure, Category="Everward|Simulation")
    TArray<FEverwardProbeCapability> GetInstalledCapabilities() const;

    UFUNCTION(BlueprintPure, Category="Everward|Command")
    FEverwardProbeCommandResult GetLastCommandResult() const;

    // These command methods are the shared authoritative control boundary.
    // Manual UI, Blueprint, and future script/automation callers must use the
    // same methods rather than implementing parallel gameplay mechanics.
    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandSetVelocityMetersPerSecond(FVector VelocityMetersPerSecond);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandStartScan(const FString& TargetId, double DurationSeconds);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandCancelScan();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandAllocatePower(EEverwardPowerSubsystem Subsystem, double Watts);

    // Compatibility wrapper retained for existing Blueprint/source callers.
    // New control surfaces should use CommandSetVelocityMetersPerSecond so
    // command acceptance/rejection is observable.
    UFUNCTION(BlueprintCallable, Category="Everward|Simulation")
    void SetProbeVelocityMetersPerSecond(FVector VelocityMetersPerSecond);

private:
    static constexpr int64 FixedStepTicks = 16667;
    static constexpr double SimulationTicksPerSecond = 1000000.0;
    static constexpr double FixedStepSeconds = static_cast<double>(FixedStepTicks) / SimulationTicksPerSecond;
    static constexpr double MetersToCentimeters = 100.0;

    FEverwardProbeCommandResult RecordCommandResult(FName CommandId, bool bAccepted, const FString& Detail);
    void SyncOwnerTransformFromSimulation();

    double FixedStepAccumulatorSeconds = 0.0;
    int64 CommandSequence = 0;
    FEverwardProbeCommandResult LastCommandResult;
    everward::simulation::SimulationCore* Core = nullptr;
};
