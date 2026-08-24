#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProbeSimulationAdapter.generated.h"

namespace everward::simulation
{
class SimulationCore;
}

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardProbeTelemetry
{
    GENERATED_BODY()

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
    double TemperatureKelvin = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double StorageUsedKilograms = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    double StorageCapacityKilograms = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry")
    FVector VelocityMetersPerSecond = FVector::ZeroVector;

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

    UFUNCTION(BlueprintCallable, Category="Everward|Simulation")
    void SetProbeVelocityMetersPerSecond(FVector VelocityMetersPerSecond);

private:
    static constexpr int64 FixedStepTicks = 16667;
    static constexpr double SimulationTicksPerSecond = 1000000.0;
    static constexpr double FixedStepSeconds = static_cast<double>(FixedStepTicks) / SimulationTicksPerSecond;
    static constexpr double MetersToCentimeters = 100.0;

    void SyncOwnerTransformFromSimulation();

    double FixedStepAccumulatorSeconds = 0.0;
    everward::simulation::SimulationCore* Core = nullptr;
};
