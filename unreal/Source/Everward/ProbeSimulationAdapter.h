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

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    bool bHasImpactHistory = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    double LastImpactEnergyJoules = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    FString LastImpactSeverity;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    FString LastImpactSubsystem;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    double LastImpactIntegrityBefore = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    double LastImpactIntegrityAfter = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    double SensorsIntegrity = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    double PropulsionIntegrity = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    double ComputationIntegrity = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Damage")
    double ThermalIntegrity = 1.0;

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
    double IntegrityFraction = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Capability")
    FString StatusReason;
};

// Slice 6 foundation: authoritative Port/Starboard manipulator arm state.
// This mirrors src/simulation/include/everward/simulation/manipulator.hpp's
// ManipulatorArmState field-for-field so the HUD/Blueprint layer never has to
// re-derive mechanical truth that already exists engine-independently.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardManipulatorArmState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    EEverwardManipulatorArmId ArmId = EEverwardManipulatorArmId::Port;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    bool bIsDeployed = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    bool bIsDeploying = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    bool bIsStowing = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double DeploymentFraction = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double ShoulderDegrees = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double ElbowDegrees = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double WristDegrees = 0.0;

    // Commanded joint targets, distinct from the current (slewing) angles
    // above. Joint articulation input reads these back rather than the
    // in-motion angle so repeated nudges accumulate against the last
    // commanded target instead of the transient current pose.
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double CommandedShoulderDegrees = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double CommandedElbowDegrees = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double CommandedWristDegrees = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    bool bToolAttached = false;

    // Slice 7 "grasp or dock with a simple object". Mirrors
    // ManipulatorArmState::grasped_target_body_id: bTargetGrasped is false
    // and GraspedTargetId is empty whenever the arm is not currently
    // holding anything.
    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    bool bTargetGrasped = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    FString GraspedTargetId;
};

// Slice 7 "approach" step: classifies ClosingSpeedMetersPerSecond into the
// qualitative motion state a player actually reads at a glance. Mirrors
// target_selection.hpp's ApproachMotionState -- presentation-classification
// only, no new physics or authoritative state.
UENUM(BlueprintType)
enum class EEverwardApproachMotion : uint8
{
    Closing UMETA(DisplayName="Closing"),
    HoldingRange UMETA(DisplayName="Holding Range"),
    Opening UMETA(DisplayName="Opening")
};

// Slice 7 foundation: authoritative selected-target range/closing-speed
// telemetry over the registered StaticSphereBody list. Mirrors
// target_selection.hpp's TargetRangeTelemetry, plus bHasSelection so "no
// target selected" and "selected target since deregistered" are both
// representable without a sentinel/empty-string convention leaking here.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardTargetSelectionStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Target")
    bool bHasSelection = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Target")
    FString TargetId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Target")
    double SurfaceRangeMeters = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Target")
    double ClosingSpeedMetersPerSecond = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Target")
    EEverwardApproachMotion ApproachMotion = EEverwardApproachMotion::HoldingRange;
};

// Slice 7 "align a manipulator" minimum interaction (PHASE2_VERTICAL_SLICE_PLAN.md):
// read-only telemetry over the same authoritative wrist forward-kinematics and
// registered-body geometry the arm/environment collision guard and target
// selection already use. bHasResult is false (and every other field is a
// meaningless default) whenever there is no selected target, the selected
// target has since been deregistered, or the queried arm is not fully
// deployed -- this never fabricates a reading. No grasp/attach/dock state is
// introduced by this struct or its accessor.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardManipulatorReachStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    bool bHasResult = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    bool bInReach = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double WristRangeToSurfaceMeters = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Manipulator")
    double RemainingDistanceMeters = 0.0;
};

// First scan-to-mining read model. This reports the bootstrap deposit without
// moving mining mechanics into Unreal; `MiningSystem` remains engine-independent.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardMiningStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining")
    FString TargetId;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining")
    FString MaterialName;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining")
    bool bSurveyed = false;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining")
    double DepositRemainingKilograms = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining")
    double ExtractedMaterialKilograms = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining")
    double ExtractionKilogramsPerCycle = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Everward|Mining")
    double ToolWorkingReachMeters = 0.0;
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

    UFUNCTION(BlueprintPure, Category="Everward|Manipulator")
    TArray<FEverwardManipulatorArmState> GetManipulatorArmStates() const;

    UFUNCTION(BlueprintPure, Category="Everward|Manipulator")
    FEverwardManipulatorReachStatus GetManipulatorReachStatus(EEverwardManipulatorArmId ArmId) const;

    UFUNCTION(BlueprintPure, Category="Everward|Mining")
    FEverwardMiningStatus GetMiningStatus() const;

    UFUNCTION(BlueprintPure, Category="Everward|Target")
    FEverwardTargetSelectionStatus GetSelectedTargetStatus() const;

    // Slice 7 "move": the authoritative current world position (meters) of
    // any registered physical body, not only the currently selected one --
    // TickComponent already writes a grasped body's authoritative center_m
    // to follow the holding arm's wrist each tick (manipulator_move.hpp's
    // grasped_target_position), so presentation code that wants a body's
    // mesh to visually follow reads it back through here rather than
    // inventing a second position source. Returns false (OutPositionMeters
    // untouched) when BodyId is not currently registered.
    UFUNCTION(BlueprintPure, Category="Everward|Target")
    bool GetStaticBodyPositionMeters(const FString& BodyId, FVector& OutPositionMeters) const;

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
    FEverwardProbeCommandResult CommandMineBootstrapTarget();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandSelectNearestTarget(double MaxSelectionRangeMeters);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandCycleTarget(double MaxSelectionRangeMeters);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandSelectTarget(const FString& TargetId);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandClearTargetSelection();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandAllocatePower(EEverwardPowerSubsystem Subsystem, double Watts);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandInstallBasicSurvivalPolicy();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandClearSoftwarePolicy();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandDeployManipulatorArm(EEverwardManipulatorArmId ArmId);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandStowManipulatorArm(EEverwardManipulatorArmId ArmId);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandSetManipulatorJointTargetDegrees(
        EEverwardManipulatorArmId ArmId, EEverwardManipulatorJoint Joint, double TargetDegrees);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandAttachManipulatorTool(EEverwardManipulatorArmId ArmId);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandDetachManipulatorTool(EEverwardManipulatorArmId ArmId);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandGraspSelectedTarget(EEverwardManipulatorArmId ArmId);

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandReleaseGraspedTarget(EEverwardManipulatorArmId ArmId);

    // Wires save_data.hpp's already-ctest-verified engine-independent
    // round trip to an actual player-facing save/load surface: a single
    // human-inspectable JSON file under Saved/SaveGames/. A rejected
    // load (missing file, unsupported save_version, malformed/inconsistent
    // state) leaves Core/Manipulators completely untouched -- see the .cpp
    // for the fail-closed construction order.
    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandSaveGame();

    UFUNCTION(BlueprintCallable, Category="Everward|Command")
    FEverwardProbeCommandResult CommandLoadGame();

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
    FString LastStartedScanTargetId;
    bool bBootstrapResourceSurveyed = false;
    double BootstrapDepositRemainingKilograms = 250.0;
    double BootstrapExtractedMaterialKilograms = 0.0;
    everward::simulation::DamageAwareProbeRuntime* Core = nullptr;
    everward::simulation::ManipulatorRig* Manipulators = nullptr;
};