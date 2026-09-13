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

// Slice 11 ("Science as gameplay") foundation: unknown -> observed ->
// characterized, matching everward::simulation::KnowledgeLevel exactly.
// Composition/material classification (the "characterized" driver) remains
// later work; this foundation only ever reaches Observed today.
UENUM(BlueprintType)
enum class EEverwardKnowledgeLevel : uint8
{
    Unknown UMETA(DisplayName="Unknown"),
    Observed UMETA(DisplayName="Observed"),
    Characterized UMETA(DisplayName="Characterized")
};

// Read-only mirror of everward::simulation::TargetKnowledgeState for
// whichever target GetSelectedTargetStatus() currently reports selected.
// bHasKnowledge is false with no selection or a selection never yet
// observed, matching FEverwardTargetSelectionStatus's own fail-closed
// contract rather than fabricating an Unknown-level reading.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardTargetKnowledgeStatus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") bool bHasKnowledge = false;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") EEverwardKnowledgeLevel Level = EEverwardKnowledgeLevel::Unknown;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") double Confidence = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") double ActiveScanSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Target") FString Classification;
};

// Slice 12 foundation follow-up: read-only mirror of one
// everward::simulation::ProbeStateSnapshot::material_inventory_kg entry.
// Core already enforces that the sum of these entries equals
// storage_used_kg (the STORAGE row's own source), so this never duplicates
// or fabricates mass -- it only exposes the existing breakdown by material
// identity that the STORAGE row's single aggregate number cannot show.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardMaterialInventoryEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") FString MaterialId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Telemetry") double Kilograms = 0.0;
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

// Issue #239: why the José fixed-step governor's notice carries its own
// stop-reason enum instead of reusing EEverwardJoseGuidanceOutcome -- the
// controller previously distinguished a fifth case
// (GetSelectedTargetStatus() no longer reports the same destination the
// player selected) that jose_guidance_command_for_body() never sees at all,
// plus the velocity command itself being rejected after a Continue outcome.
// Reusing the guidance enum would either conflate those two cases into
// "Destination Not Found" or silently drop one, changing the exact
// player-facing message this fix must not change.
UENUM(BlueprintType)
enum class EEverwardJoseAutopilotStopReason : uint8
{
    None UMETA(DisplayName="None"),
    SelectionChanged UMETA(DisplayName="Selection Changed"),
    Arrived UMETA(DisplayName="Arrived"),
    DestinationUnresolved UMETA(DisplayName="Destination Unresolved"),
    DestinationNotFound UMETA(DisplayName="Destination Not Found"),
    CommandRejected UMETA(DisplayName="Command Rejected")
};

// Mirrors FEverwardAutomationNotice's sequence-number change-detection
// pattern (GetControlledHoverGovernorNotice()) but also carries which of the
// five stop reasons above ended the session, and the destination id that was
// active, so AdvanceJoseTakeTheWheel() can still show the exact same
// player-facing message it did when it computed the outcome itself.
USTRUCT(BlueprintType)
struct EVERWARD_API FEverwardJoseAutopilotGovernorNotice
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Everward|Autopilot") int64 Sequence = 0;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Autopilot") EEverwardJoseAutopilotStopReason StopReason = EEverwardJoseAutopilotStopReason::None;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Autopilot") FString DestinationId;
    UPROPERTY(BlueprintReadOnly, Category="Everward|Autopilot") FString Detail;
};

// Slice 10 near-surface command shaping (surface_descent_guidance.hpp):
// constrains a requested inertial velocity to a body-relative controlled-
// descent envelope around whatever planetary body SimulationCore currently
// has registered (Slice 9's set_planetary_body/clear_planetary_body).
// bHasResult is false whenever no planetary body is registered -- this
// never fabricates a descent envelope for deep space, mirroring
// FEverwardJoseGuidanceCommand's DestinationNotFound fail-closed contract.
// Also reused by GetControlledHoverVelocityCommand()/
// CommandSetControlledHoverVelocityMetersPerSecond(): both queries return the
// same shape (a constrained velocity plus which axis, if any, was clamped),
// so no second Blueprint-visible result struct is introduced for hover.
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
    UFUNCTION(BlueprintPure, Category="Everward|Telemetry") TArray<FEverwardMaterialInventoryEntry> GetStoredMaterialInventory() const;
    UFUNCTION(BlueprintPure, Category="Everward|Target") FEverwardTargetSelectionStatus GetSelectedTargetStatus() const;
    UFUNCTION(BlueprintPure, Category="Everward|Target") FEverwardTargetKnowledgeStatus GetSelectedTargetKnowledgeStatus() const;
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
    UFUNCTION(BlueprintPure, Category="Everward|Descent") FEverwardControlledDescentCommand GetControlledHoverVelocityCommand(
        FVector RequestedVelocityMetersPerSecond,
        double MaxTangentialSpeedMetersPerSecond,
        double MinimumClearanceMeters,
        double TargetAltitudeMeters,
        double AltitudeGainPerSecond,
        double MaxVerticalCorrectionSpeedMetersPerSecond) const;
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
    UFUNCTION(BlueprintCallable, Category="Everward|Command") FEverwardProbeCommandResult CommandSetControlledHoverVelocityMetersPerSecond(
        FVector RequestedVelocityMetersPerSecond,
        double MaxTangentialSpeedMetersPerSecond,
        double MinimumClearanceMeters,
        double TargetAltitudeMeters,
        double AltitudeGainPerSecond,
        double MaxVerticalCorrectionSpeedMetersPerSecond);
    // Issue #235 finding 2: the controller only configures this governor;
    // the correction itself is re-applied by AdvanceControlledHoverGovernorFixedStep()
    // once per elapsed authoritative fixed step inside TickComponent()'s own
    // accumulator loop, so its cadence tracks the simulation's tick sequence
    // rather than the render frame rate. Engaging with bEngaged=false (e.g.
    // from CancelControlledHover()) stops the fixed-step correction
    // immediately without waiting for a rejection.
    UFUNCTION(BlueprintCallable, Category="Everward|Command") void SetControlledHoverGovernorEngaged(
        bool bEngaged,
        double MaxTangentialSpeedMetersPerSecond,
        double MinimumClearanceMeters,
        double TargetAltitudeMeters,
        double AltitudeGainPerSecond,
        double MaxVerticalCorrectionSpeedMetersPerSecond);
    // Lets the controller detect a fixed-step rejection (e.g. the registered
    // planetary body going away mid-hover) without polling Core directly;
    // mirrors GetLastAutomationNotice()'s sequence-number change-detection.
    UFUNCTION(BlueprintPure, Category="Everward|Command") FEverwardAutomationNotice GetControlledHoverGovernorNotice() const;
    // Issue #239: same fixed-step governor pattern as controlled hover
    // (#235/#238) applied to controlled descent -- the controller only
    // configures the governor; AdvanceControlledDescentGovernorFixedStep()
    // re-applies the correction once per elapsed authoritative fixed step
    // from inside TickComponent()'s own accumulator loop.
    UFUNCTION(BlueprintCallable, Category="Everward|Command") void SetControlledDescentGovernorEngaged(
        bool bEngaged,
        double MaxDescentSpeedMetersPerSecond,
        double MaxTangentialSpeedMetersPerSecond,
        double MinimumClearanceMeters,
        double FullSpeedAltitudeMeters,
        double TouchdownDescentSpeedMetersPerSecond);
    UFUNCTION(BlueprintPure, Category="Everward|Command") FEverwardAutomationNotice GetControlledDescentGovernorNotice() const;
    // Issue #239: José Take the Wheel is a destination autopilot rather than
    // a plain velocity governor, but the same render-cadence defect applied
    // -- AdvanceJoseTakeTheWheel() computed guidance and issued the resulting
    // velocity command directly from APlayerController::Tick(). The
    // controller now only configures the destination/tunables here;
    // AdvanceJoseAutopilotGovernorFixedStep() re-evaluates guidance and
    // issues/stops the command once per elapsed authoritative fixed step.
    UFUNCTION(BlueprintCallable, Category="Everward|Autopilot") void SetJoseAutopilotGovernorEngaged(
        bool bEngaged,
        const FString& DestinationBodyId,
        double CruiseSpeedMetersPerSecond,
        double ArrivalSurfaceStandoffMeters,
        double ArrivalToleranceMeters,
        double ApproachGainPerSecond);
    UFUNCTION(BlueprintPure, Category="Everward|Autopilot") FEverwardJoseAutopilotGovernorNotice GetJoseAutopilotGovernorNotice() const;
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
    // Called once per elapsed fixed step from inside TickComponent()'s own
    // accumulator loop -- see SetControlledHoverGovernorEngaged()'s comment.
    void AdvanceControlledHoverGovernorFixedStep();
    // Issue #239: same fixed-step call site pattern, for controlled descent
    // and José -- see SetControlledDescentGovernorEngaged()'s and
    // SetJoseAutopilotGovernorEngaged()'s comments.
    void AdvanceControlledDescentGovernorFixedStep();
    void AdvanceJoseAutopilotGovernorFixedStep();

    double FixedStepAccumulatorSeconds = 0.0;
    double TractorStepAccumulatorSeconds = 0.0;
    int64 CommandSequence = 0;
    int64 AutomationSequence = 0;
    int64 ScanLifecycleSequence = 0;
    FEverwardProbeCommandResult LastCommandResult;
    FEverwardAutomationNotice LastAutomationNotice;

    bool bControlledHoverGovernorEngaged = false;
    double ControlledHoverGovernorMaxTangentialSpeedMetersPerSecond = 0.0;
    double ControlledHoverGovernorMinimumClearanceMeters = 0.0;
    double ControlledHoverGovernorTargetAltitudeMeters = 0.0;
    double ControlledHoverGovernorAltitudeGainPerSecond = 0.0;
    double ControlledHoverGovernorMaxVerticalCorrectionSpeedMetersPerSecond = 0.0;
    int64 ControlledHoverGovernorSequence = 0;
    FEverwardAutomationNotice LastControlledHoverGovernorNotice;

    bool bControlledDescentGovernorEngaged = false;
    double ControlledDescentGovernorMaxDescentSpeedMetersPerSecond = 0.0;
    double ControlledDescentGovernorMaxTangentialSpeedMetersPerSecond = 0.0;
    double ControlledDescentGovernorMinimumClearanceMeters = 0.0;
    double ControlledDescentGovernorFullSpeedAltitudeMeters = 0.0;
    double ControlledDescentGovernorTouchdownSpeedMetersPerSecond = 0.0;
    int64 ControlledDescentGovernorSequence = 0;
    FEverwardAutomationNotice LastControlledDescentGovernorNotice;

    bool bJoseAutopilotGovernorEngaged = false;
    FString JoseAutopilotGovernorDestinationBodyId;
    double JoseAutopilotGovernorCruiseSpeedMetersPerSecond = 0.0;
    double JoseAutopilotGovernorArrivalSurfaceStandoffMeters = 0.0;
    double JoseAutopilotGovernorArrivalToleranceMeters = 0.0;
    double JoseAutopilotGovernorApproachGainPerSecond = 0.0;
    int64 JoseAutopilotGovernorSequence = 0;
    FEverwardJoseAutopilotGovernorNotice LastJoseAutopilotGovernorNotice;

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
