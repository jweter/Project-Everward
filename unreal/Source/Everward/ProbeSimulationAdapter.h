#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProbeSimulationAdapter.generated.h"

namespace everward::simulation
{
class ProbeRuntime;
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
struct EVERWARD_API FEverwardAutomationNotice
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Automation")
    int64 Sequence = 0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Automation")
    bool bRejected = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Automation")
    FString Detail;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardScanLifecycleNotice
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Scan")
    int64 Sequence = 0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Scan")
    bool bCompleted = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Scan")
    bool bCancelled = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Scan")
    FString Detail;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardSoftwarePolicyStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Policy")
    bool bInstalled = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Policy")
    bool bEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Policy")
    bool bExecutorAvailable = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Policy")
    FString PolicyId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Policy")
    int32 RuleCount = 0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Policy")
    double MinimumComputationPowerWatts = 0.0;
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

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    double CollisionEnvelopeRadiusMeters = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    bool bHasContactHistory = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    FString LastContactBodyId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    FVector LastContactPointMeters = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    FVector LastContactSurfaceNormal = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    FVector LastContactRelativeVelocityMetersPerSecond = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    double LastContactNormalSpeedMetersPerSecond = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact")
    int64 LastContactTick = 0;

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
    FRotator AttitudeDegrees = FRotator::ZeroRotator;

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

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    double MinimumOperatingPowerWatts = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    FString StatusReason;
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

    UFUNCTION(BlueprintPure, Category="Everward|Policy")
    FEverwardSoftwarePolicyStatus GetSoftwarePolicyStatus() const;

    UFUNCTION(BlueprintPure, Category="Everward|Command")
    FEverwardProbeCommandResult GetLastCommandResult() const;

    UFUNCTION(BlueprintPure, Category="Everward|Automation")
    FEverwardAutomationNotice GetLastAutomationNotice() const;

    UFUNCTION(BlueprintPure, Category="Everward|Scan")
    FEverwardScanLifecycleNotice GetLastScanLifecycleNotice() const;

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandSetVelocityMetersPerSecond(FVector VelocityMetersPerSecond);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandAdjustLocalVelocityMetersPerSecond(FVector DeltaLocalVelocityMetersPerSecond);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandAdjustAttitudeDegrees(FRotator DeltaAttitudeDegrees);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandStartScan(const FString& TargetId, double DurationSeconds);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandCancelScan();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandAllocatePower(EEverwardPowerSubsystem Subsystem, double Watts);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandInstallBasicSurvivalPolicy();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandClearSoftwarePolicy();

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
    int64 AutomationSequence = 0;
    int64 ScanLifecycleSequence = 0;
    FEverwardProbeCommandResult LastCommandResult;
    FEverwardAutomationNotice LastAutomationNotice;
    FEverwardScanLifecycleNotice LastScanLifecycleNotice;
    everward::simulation::ProbeRuntime* Core = nullptr;
};