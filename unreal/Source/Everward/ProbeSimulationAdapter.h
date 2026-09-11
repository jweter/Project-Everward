#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProbeSimulationAdapter.generated.h"

namespace everward::simulation
{
class DamageAwareProbeRuntime;
class ManipulatorRig;
}

UENUM(BlueprintType)
enum class EEverwardPowerSubsystem : uint8
{
    Sensors UMETA(DisplayName="Sensors"),
    Propulsion UMETA(DisplayName="Propulsion"),
    Computation UMETA(DisplayName="Computation"),
    Thermal UMETA(DisplayName="Thermal")
};

UENUM(BlueprintType)
enum class EEverwardManipulatorArmId : uint8
{
    Port UMETA(DisplayName="Port"),
    Starboard UMETA(DisplayName="Starboard")
};

UENUM(BlueprintType)
enum class EEverwardManipulatorJoint : uint8
{
    Shoulder UMETA(DisplayName="Shoulder"),
    Elbow UMETA(DisplayName="Elbow"),
    Wrist UMETA(DisplayName="Wrist")
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

    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") FString ProbeId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") int32 Generation = 0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") int64 SimulationTick = 0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double SimulationTimeSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double MassKilograms = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") double CollisionEnvelopeRadiusMeters = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") bool bHasContactHistory = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") FString LastContactBodyId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") FVector LastContactPointMeters = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") FVector LastContactSurfaceNormal = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") FVector LastContactRelativeVelocityMetersPerSecond = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") double LastContactNormalSpeedMetersPerSecond = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Contact") int64 LastContactTick = 0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") bool bHasImpactHistory = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") double LastImpactEnergyJoules = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") FString LastImpactSeverity;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") FString LastImpactSubsystem;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") double LastImpactIntegrityBefore = 1.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") double LastImpactIntegrityAfter = 1.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") double SensorsIntegrity = 1.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") double PropulsionIntegrity = 1.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") double ComputationIntegrity = 1.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage") double ThermalIntegrity = 1.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double StoredEnergyJoules = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double EnergyCapacityJoules = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double EnergyGenerationWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double PowerCapacityWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double PowerAllocatedSensorsWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double PowerAllocatedPropulsionWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double PowerAllocatedComputationWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double PowerAllocatedThermalWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double TotalPowerAllocatedWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double TemperatureKelvin = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double StorageUsedKilograms = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double StorageCapacityKilograms = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") FVector VelocityMetersPerSecond = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") FRotator AttitudeDegrees = FRotator::ZeroRotator;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") bool bIsScanning = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") FString ActiveScanTargetId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double ScanRemainingSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") bool bIsOverheated = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") bool bIsEnergyDepleted = false;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardProbeCapability
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") FName CapabilityId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") FString DisplayName;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") FString Description;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") bool bInstalled = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") bool bOperational = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") bool bSupportsManualControl = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") bool bSupportsAutomation = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") double AllocatedPowerWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") double MinimumOperatingPowerWatts = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") double IntegrityFraction = 1.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability") FString StatusReason;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardManipulatorArmState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") EEverwardManipulatorArmId ArmId = EEverwardManipulatorArmId::Port;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") bool bIsDeployed = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") bool bIsDeploying = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") bool bIsStowing = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double DeploymentFraction = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double ShoulderDegrees = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double ElbowDegrees = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double WristDegrees = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double CommandedShoulderDegrees = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double CommandedElbowDegrees = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double CommandedWristDegrees = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") bool bToolAttached = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") bool bTargetGrasped = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") FString GraspedTargetId;
};

UENUM(BlueprintType)
enum class EEverwardApproachMotion : uint8
{
    Closing UMETA(DisplayName="Closing"),
    HoldingRange UMETA(DisplayName="Holding Range"),
    Opening UMETA(DisplayName="Opening")
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardTargetSelectionStatus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") bool bHasSelection = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") FString TargetId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") double SurfaceRangeMeters = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") double ClosingSpeedMetersPerSecond = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") EEverwardApproachMotion ApproachMotion = EEverwardApproachMotion::HoldingRange;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardTractorFieldStatus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") bool bEngaged = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") bool bHasTarget = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") FString TargetId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") double ProbeMassKilograms = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") double TargetMassKilograms = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") double TargetToProbeMassRatio = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") double SurfaceRangeMeters = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") double MaxSurfaceRangeMeters = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") double RequestedForceNewtons = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") double AppliedForceNewtons = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") FVector TargetVelocityMetersPerSecond = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") FString ExpectedMotion;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Tractor") FString Detail;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardManipulatorReachStatus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") bool bHasResult = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") bool bInReach = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double WristRangeToSurfaceMeters = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator") double RemainingDistanceMeters = 0.0;
};

UENUM(BlueprintType)
enum class EEverwardJoseGuidanceOutcome : uint8
{
    Continue UMETA(DisplayName="Continue"),
    Arrived UMETA(DisplayName="Arrived"),
    DestinationUnresolved UMETA(DisplayName="Destination Unresolved"),
    DestinationNotFound UMETA(DisplayName="Destination Not Found")
};

// docs/JOSE_TAKE_THE_WHEEL.md guidance-law result: the authoritative
// engine-independent decision (jose_autopilot.hpp), not a raw telemetry
// read. Unreal's autopilot controller only interprets Outcome and, on
// Continue, issues CommandVelocityMetersPerSecond through the existing
// CommandSetVelocityMetersPerSecond() boundary -- it does not compute the
// heading or approach speed itself.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardJoseGuidanceCommand
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Autopilot") EEverwardJoseGuidanceOutcome Outcome = EEverwardJoseGuidanceOutcome::DestinationNotFound;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Autopilot") FVector CommandVelocityMetersPerSecond = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Autopilot") double RemainingSurfaceRangeMeters = 0.0;
};

// Slice 10 near-surface command shaping (surface_descent_guidance.hpp):
// constrains a requested inertial velocity to a body-relative controlled-
// descent envelope around whatever planetary body SimulationCore currently
// has registered (Slice 9's set_planetary_body/clear_planetary_body).
// bHasResult is false whenever no planetary body is registered -- this
// never fabricates a descent envelope for deep space, mirroring
// FEverwardJoseGuidanceCommand's DestinationNotFound fail-closed contract.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardControlledDescentCommand
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Descent") bool bHasResult = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Descent") FVector CommandVelocityMetersPerSecond = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Descent") bool bDescentRateLimited = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Descent") bool bTangentialRateLimited = false;
};

USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardMiningStatus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining") FString TargetId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining") FString MaterialName;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining") bool bSurveyed = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining") double DepositRemainingKilograms = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining") double ExtractedMaterialKilograms = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining") double ExtractionKilogramsPerCycle = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining") double ToolWorkingReachMeters = 0.0;
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

    UFUNCTION(BlueprintPure, Category="Everward|Simulation") int64 GetSimulationTick() const;
    UFUNCTION(BlueprintPure, Category="Everward|Simulation") FVector GetProbePositionMeters() const;
    UFUNCTION(BlueprintPure, Category="Everward|Simulation") FEverwardProbeTelemetry GetProbeTelemetry() const;
    UFUNCTION(BlueprintPure, Category="Everward|Simulation") TArray<FEverwardProbeCapability> GetInstalledCapabilities() const;
    UFUNCTION(BlueprintPure, Category="Everward|Policy") FEverwardSoftwarePolicyStatus GetSoftwarePolicyStatus() const;
    UFUNCTION(BlueprintPure, Category="Everward|Manipulator") TArray<FEverwardManipulatorArmState> GetManipulatorArmStates() const;
    UFUNCTION(BlueprintPure, Category="Everward|Manipulator") FEverwardManipulatorReachStatus GetManipulatorReachStatus(EEverwardManipulatorArmId ArmId) const;
    UFUNCTION(BlueprintPure, Category="Everward|Mining") FEverwardMiningStatus GetMiningStatus() const;
    UFUNCTION(BlueprintPure, Category="Everward|Target") FEverwardTargetSelectionStatus GetSelectedTargetStatus() const;
    UFUNCTION(BlueprintPure, Category="Everward|Tractor") FEverwardTractorFieldStatus GetTractorFieldStatus() const;
    UFUNCTION(BlueprintPure, Category="Everward|Target") bool GetStaticBodyPositionMeters(const FString& BodyId, FVector& OutPositionMeters) const;
    UFUNCTION(BlueprintPure, Category="Everward|Autopilot") FEverwardJoseGuidanceCommand GetJoseGuidanceCommand(
        const FString& DestinationBodyId,
        double CruiseSpeedMetersPerSecond,
        double ArrivalSurfaceStandoffMeters,
        double ArrivalToleranceMeters,
        double ApproachGainPerSecond) const;
    UFUNCTION(BlueprintPure, Category="Everward|Descent") FEverwardControlledDescentCommand GetControlledDescentVelocityCommand(
        FVector RequestedVelocityMetersPerSecond,
        double MaxDescentSpeedMetersPerSecond,
        double MaxTangentialSpeedMetersPerSecond,
        double MinimumClearanceMeters,
        double FullSpeedAltitudeMeters,
        double TouchdownDescentSpeedMetersPerSecond) const;
    UFUNCTION(BlueprintPure, Category="Everward|Command") FEverwardProbeCommandResult GetLastCommandResult() const;
    UFUNCTION(BlueprintPure, Category="Everward|Automation") FEverwardAutomationNotice GetLastAutomationNotice() const;
    UFUNCTION(BlueprintPure, Category="Everward|Scan") FEverwardScanLifecycleNotice GetLastScanLifecycleNotice() const;

    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandSetVelocityMetersPerSecond(FVector VelocityMetersPerSecond);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandSetControlledDescentVelocityMetersPerSecond(
        FVector RequestedVelocityMetersPerSecond,
        double MaxDescentSpeedMetersPerSecond,
        double MaxTangentialSpeedMetersPerSecond,
        double MinimumClearanceMeters,
        double FullSpeedAltitudeMeters,
        double TouchdownDescentSpeedMetersPerSecond);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandAdjustLocalVelocityMetersPerSecond(FVector DeltaLocalVelocityMetersPerSecond);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandAdjustAttitudeDegrees(FRotator DeltaAttitudeDegrees);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandStartScan(const FString& TargetId, double DurationSeconds);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandCancelScan();
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandMineBootstrapTarget();
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandSelectNearestTarget(double MaxSelectionRangeMeters);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandCycleTarget(double MaxSelectionRangeMeters);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandSelectTarget(const FString& TargetId);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandClearTargetSelection();
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandEngageTractorField(double RequestedForceNewtons);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandDisengageTractorField();
    void AdvanceTractorField(double DeltaSeconds);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandAllocatePower(EEverwardPowerSubsystem Subsystem, double Watts);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandInstallBasicSurvivalPolicy();
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandClearSoftwarePolicy();
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandDeployManipulatorArm(EEverwardManipulatorArmId ArmId);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandStowManipulatorArm(EEverwardManipulatorArmId ArmId);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandSetManipulatorJointTargetDegrees(EEverwardManipulatorArmId ArmId, EEverwardManipulatorJoint Joint, double TargetDegrees);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandAttachManipulatorTool(EEverwardManipulatorArmId ArmId);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandDetachManipulatorTool(EEverwardManipulatorArmId ArmId);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandGraspSelectedTarget(EEverwardManipulatorArmId ArmId);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandReleaseGraspedTarget(EEverwardManipulatorArmId ArmId);
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandSaveGame();
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandLoadGame();
    UFUNCTION(BlueprintCallable, Category="Everward|Simulation") void SetProbeVelocityMetersPerSecond(FVector VelocityMetersPerSecond);

private:
    friend class AFixItRuntimeActor;

    static constexpr int64 FixedStepTicks = 16667;
    static constexpr double SimulationTicksPerSecond = 1000000.0;
    static constexpr double FixedStepSeconds = static_cast<double>(FixedStepTicks) / SimulationTicksPerSecond;
    static constexpr double MetersToCentimeters = 100.0;
    static constexpr double TractorMaxSurfaceRangeMeters = 40.0;
    static constexpr double TractorMaxForceNewtons = 5000.0;

    FEverwardProbeCommandResult RecordCommandResult(FName CommandId, bool bAccepted, const FString& Detail);
    void SyncOwnerTransformFromSimulation();

    double FixedStepAccumulatorSeconds = 0.0;
    double TractorStepAccumulatorSeconds = 0.0;
    int64 CommandSequence = 0;
    int64 AutomationSequence = 0;
    int64 ScanLifecycleSequence = 0;
    FEverwardProbeCommandResult LastCommandResult;
    FEverwardAutomationNotice LastAutomationNotice;
    FEverwardScanLifecycleNotice LastScanLifecycleNotice;
    FString LastStartedScanTargetId;
    bool bBootstrapResourceSurveyed = false;
    double BootstrapDepositRemainingKilograms = 250.0;
    double BootstrapExtractedMaterialKilograms = 0.0;

    bool bTractorFieldEngaged = false;
    FString TractorTargetId;
    double TractorRequestedForceNewtons = 0.0;
    double TractorAppliedForceNewtons = 0.0;
    FString TractorFieldDetail;
    TMap<FString, FVector> TractorTargetVelocitiesMetersPerSecond;

    everward::simulation::DamageAwareProbeRuntime* Core = nullptr;
    everward::simulation::ManipulatorRig* Manipulators = nullptr;
};
