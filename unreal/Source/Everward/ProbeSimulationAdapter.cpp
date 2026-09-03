#include "ProbeSimulationAdapter.h"

#include "EverwardPhase2TestEnvironment.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "everward/simulation/software_policy.hpp"
#include "everward/simulation/impact_damage.hpp"
#include "everward/simulation/manipulator.hpp"
#include "everward/simulation/manipulator_hull_contact.hpp"
#include "everward/simulation/manipulator_reach.hpp"
#include "everward/simulation/manipulator_grasp.hpp"
#include "everward/simulation/manipulator_move.hpp"
#include "everward/simulation/manipulator_release.hpp"
#include "everward/simulation/save_data.hpp"

#include <exception>
#include <string>

namespace
{
everward::simulation::PowerSubsystem ToSimulationPowerSubsystem(EEverwardPowerSubsystem Subsystem)
{
    switch (Subsystem)
    {
        case EEverwardPowerSubsystem::Sensors: return everward::simulation::PowerSubsystem::Sensors;
        case EEverwardPowerSubsystem::Propulsion: return everward::simulation::PowerSubsystem::Propulsion;
        case EEverwardPowerSubsystem::Computation: return everward::simulation::PowerSubsystem::Computation;
        case EEverwardPowerSubsystem::Thermal: return everward::simulation::PowerSubsystem::Thermal;
    }
    return everward::simulation::PowerSubsystem::Sensors;
}

const TCHAR* PowerSubsystemName(EEverwardPowerSubsystem Subsystem)
{
    switch (Subsystem)
    {
        case EEverwardPowerSubsystem::Sensors: return TEXT("sensors");
        case EEverwardPowerSubsystem::Propulsion: return TEXT("propulsion");
        case EEverwardPowerSubsystem::Computation: return TEXT("computation");
        case EEverwardPowerSubsystem::Thermal: return TEXT("thermal");
    }
    return TEXT("unknown");
}

const TCHAR* SimulationSubsystemName(everward::simulation::PowerSubsystem Subsystem)
{
    switch (Subsystem)
    {
        case everward::simulation::PowerSubsystem::Sensors: return TEXT("SENSORS");
        case everward::simulation::PowerSubsystem::Propulsion: return TEXT("PROPULSION");
        case everward::simulation::PowerSubsystem::Computation: return TEXT("COMPUTATION");
        case everward::simulation::PowerSubsystem::Thermal: return TEXT("THERMAL");
    }
    return TEXT("UNKNOWN");
}

FString SharedProbeLockoutReason(const everward::simulation::ProbeStateSnapshot& Snapshot)
{
    if (Snapshot.is_energy_depleted) return TEXT("ENERGY DEPLETED");
    if (Snapshot.is_overheated) return TEXT("THERMAL LOCKOUT");
    return FString();
}

everward::simulation::ManipulatorArmId ToSimulationManipulatorArmId(EEverwardManipulatorArmId ArmId)
{
    switch (ArmId)
    {
        case EEverwardManipulatorArmId::Port: return everward::simulation::ManipulatorArmId::Port;
        case EEverwardManipulatorArmId::Starboard: return everward::simulation::ManipulatorArmId::Starboard;
    }
    return everward::simulation::ManipulatorArmId::Port;
}

const TCHAR* ManipulatorArmName(EEverwardManipulatorArmId ArmId)
{
    switch (ArmId)
    {
        case EEverwardManipulatorArmId::Port: return TEXT("port");
        case EEverwardManipulatorArmId::Starboard: return TEXT("starboard");
    }
    return TEXT("unknown");
}

everward::simulation::ManipulatorJoint ToSimulationManipulatorJoint(EEverwardManipulatorJoint Joint)
{
    switch (Joint)
    {
        case EEverwardManipulatorJoint::Shoulder: return everward::simulation::ManipulatorJoint::Shoulder;
        case EEverwardManipulatorJoint::Elbow: return everward::simulation::ManipulatorJoint::Elbow;
        case EEverwardManipulatorJoint::Wrist: return everward::simulation::ManipulatorJoint::Wrist;
    }
    return everward::simulation::ManipulatorJoint::Shoulder;
}

const TCHAR* ManipulatorJointName(EEverwardManipulatorJoint Joint)
{
    switch (Joint)
    {
        case EEverwardManipulatorJoint::Shoulder: return TEXT("shoulder");
        case EEverwardManipulatorJoint::Elbow: return TEXT("elbow");
        case EEverwardManipulatorJoint::Wrist: return TEXT("wrist");
    }
    return TEXT("unknown");
}

FEverwardManipulatorArmState ToUnrealManipulatorArmState(
    EEverwardManipulatorArmId ArmId,
    const everward::simulation::ManipulatorArmState& State)
{
    FEverwardManipulatorArmState Out;
    Out.ArmId = ArmId;
    Out.bIsDeployed = State.is_deployed;
    Out.bIsDeploying = State.is_deploying;
    Out.bIsStowing = State.is_stowing;
    Out.DeploymentFraction = State.deployment_fraction;
    Out.ShoulderDegrees = State.angles.shoulder_degrees;
    Out.ElbowDegrees = State.angles.elbow_degrees;
    Out.WristDegrees = State.angles.wrist_degrees;
    Out.CommandedShoulderDegrees = State.commanded_angles.shoulder_degrees;
    Out.CommandedElbowDegrees = State.commanded_angles.elbow_degrees;
    Out.CommandedWristDegrees = State.commanded_angles.wrist_degrees;
    Out.bToolAttached = State.tool_attached;
    Out.bTargetGrasped = !State.grasped_target_body_id.empty();
    Out.GraspedTargetId = UTF8_TO_TCHAR(State.grasped_target_body_id.c_str());
    return Out;
}

FString IntegrityReason(
    const everward::simulation::DamageAwareProbeRuntime& Runtime,
    everward::simulation::PowerSubsystem Subsystem)
{
    const double Integrity = Runtime.subsystem_integrity(Subsystem);
    const auto Band = Runtime.subsystem_integrity_band(Subsystem);
    return FString::Printf(
        TEXT("INTEGRITY %.0f%% // %s"),
        Integrity * 100.0,
        UTF8_TO_TCHAR(everward::simulation::ImpactDamageModel::integrity_band_name(Band)));
}

// Guarded against both the probe's own hull envelope and every registered
// external body (see BeginPlay's original comment, preserved here now that
// both BeginPlay and CommandLoadGame need to build a rig around whichever
// DamageAwareProbeRuntime is live at the time). The returned guard always
// reads CoreForGuard's live position/attitude/registered bodies on every
// call rather than a snapshot taken here.
everward::simulation::ManipulatorRig::SelfCollisionGuard BuildManipulatorCollisionGuard(
    everward::simulation::DamageAwareProbeRuntime* CoreForGuard)
{
    return everward::simulation::make_combined_collision_guard(
        everward::simulation::make_hull_self_collision_guard(),
        everward::simulation::make_environment_collision_guard(
            [CoreForGuard]() {
                const auto& Snapshot = CoreForGuard->snapshot();
                return everward::simulation::ProbeWorldPose{Snapshot.position_m, Snapshot.attitude_degrees};
            },
            [CoreForGuard]() -> const std::vector<everward::simulation::StaticSphereBody>& {
                return CoreForGuard->static_bodies();
            }));
}

// Single canonical location for the save file's path, shared by
// CommandSaveGame/CommandLoadGame. A single fixed slot (not per-save-name)
// matches this first prototype's single-canonical-probe scope; multiple
// save slots are later UI work, not a save-format limitation.
FString SaveGameFilePath()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("everward_save_v1.json"));
}
}

UProbeSimulationAdapter::UProbeSimulationAdapter()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UProbeSimulationAdapter::BeginPlay()
{
    Super::BeginPlay();
    Core = new everward::simulation::DamageAwareProbeRuntime(
        everward::simulation::DamageAwareProbeRuntime::make_canonical_ev0001());
    // Guarded against both the probe's own hull envelope (so a commanded
    // joint pose can never visually drive an arm through the probe's own
    // body) and every registered external body (so an arm cannot sweep into
    // an asteroid/scan target either) -- Slice 6's "collision does not allow
    // impossible penetration" requirement, extended from self-collision to
    // arm/environment collision per PROJECT_STATUS.md. CommandLoadGame
    // rebuilds an equivalent guard around the freshly restored runtime
    // through the same BuildManipulatorCollisionGuard helper.
    Manipulators = new everward::simulation::ManipulatorRig(BuildManipulatorCollisionGuard(Core));

    // Register the same sphere rendered by the temporary Phase-2 environment.
    // The runtime, not Unreal collision response, decides whether EV-0001 can
    // pass through it. Slice 4 then derives damage from that contact truth.
    Core->add_static_sphere_body({
        std::string(TCHAR_TO_UTF8(AEverwardPhase2TestEnvironment::BootstrapScanTargetId)),
        {
            AEverwardPhase2TestEnvironment::BootstrapBodyCenterXMeters,
            AEverwardPhase2TestEnvironment::BootstrapBodyCenterYMeters,
            AEverwardPhase2TestEnvironment::BootstrapBodyCenterZMeters,
        },
        AEverwardPhase2TestEnvironment::BootstrapBodyRadiusMeters,
    });

    // Slice 8 (partial): register the environment's two additional reference
    // targets the same way, so target cycling (#149/#150) has more than one
    // eligible registered body to actually cycle through -- see
    // AEverwardPhase2TestEnvironment.h's ReferenceTarget1/2 constants.
    Core->add_static_sphere_body({
        std::string(TCHAR_TO_UTF8(AEverwardPhase2TestEnvironment::ReferenceTarget1Id)),
        {
            AEverwardPhase2TestEnvironment::ReferenceTarget1CenterXMeters,
            AEverwardPhase2TestEnvironment::ReferenceTarget1CenterYMeters,
            AEverwardPhase2TestEnvironment::ReferenceTarget1CenterZMeters,
        },
        AEverwardPhase2TestEnvironment::ReferenceTarget1RadiusMeters,
    });
    Core->add_static_sphere_body({
        std::string(TCHAR_TO_UTF8(AEverwardPhase2TestEnvironment::ReferenceTarget2Id)),
        {
            AEverwardPhase2TestEnvironment::ReferenceTarget2CenterXMeters,
            AEverwardPhase2TestEnvironment::ReferenceTarget2CenterYMeters,
            AEverwardPhase2TestEnvironment::ReferenceTarget2CenterZMeters,
        },
        AEverwardPhase2TestEnvironment::ReferenceTarget2RadiusMeters,
    });

    SyncOwnerTransformFromSimulation();
}

void UProbeSimulationAdapter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    delete Core;
    Core = nullptr;
    delete Manipulators;
    Manipulators = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UProbeSimulationAdapter::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (Core == nullptr) return;

    FixedStepAccumulatorSeconds += static_cast<double>(DeltaTime);
    while (FixedStepAccumulatorSeconds >= FixedStepSeconds)
    {
        Core->advance_wall_ticks(FixedStepTicks);
        if (Manipulators != nullptr)
        {
            Manipulators->advance(FixedStepSeconds);

            // Slice 7 "move": every other registered-body reader (contact,
            // target selection, reach) already reads a body's center_m
            // straight from Core, so writing the grasping arm's authoritative
            // wrist world position back into that same field each tick is
            // what makes a held body actually follow the arm instead of
            // sitting at its original registered position. Reuses
            // manipulator_move.hpp's grasped_target_position exactly as its
            // own tests exercise it; a nullopt result (arm holds nothing)
            // leaves that body's position untouched.
            const everward::simulation::ProbeWorldPose ProbePose{
                Core->snapshot().position_m, Core->snapshot().attitude_degrees};
            for (const auto SimulationArmId :
                {everward::simulation::ManipulatorArmId::Port, everward::simulation::ManipulatorArmId::Starboard})
            {
                const auto Moved = everward::simulation::grasped_target_position(
                    SimulationArmId, Manipulators->arm(SimulationArmId), ProbePose);
                if (Moved.has_value())
                {
                    Core->update_static_sphere_body_position(Moved->body_id, Moved->world_position_m);
                }
            }
        }
        FixedStepAccumulatorSeconds -= FixedStepSeconds;
    }

    SyncOwnerTransformFromSimulation();

    const auto DamageRecords = Core->drain_damage_records();
    for (const auto& Record : DamageRecords)
    {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("Everward impact: %s %.0f J -> %s // %s %.0f%% -> %.0f%%"),
            UTF8_TO_TCHAR(everward::simulation::ImpactDamageModel::severity_name(Record.severity)),
            Record.impact_energy_j,
            *FString(UTF8_TO_TCHAR(Record.body_id.c_str())),
            SimulationSubsystemName(Record.affected_subsystem),
            Record.integrity_before * 100.0,
            Record.integrity_after * 100.0);
    }

    const auto Events = Core->drain_events();
    for (const auto& Event : Events)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Everward simulation event at tick %lld"), Event.tick);

        if (Event.type == everward::simulation::DomainEventType::PolicyRuleTriggered ||
            Event.type == everward::simulation::DomainEventType::PolicyActionRejected)
        {
            LastAutomationNotice.Sequence = ++AutomationSequence;
            LastAutomationNotice.bRejected =
                Event.type == everward::simulation::DomainEventType::PolicyActionRejected;
            LastAutomationNotice.Detail = UTF8_TO_TCHAR(Event.detail.c_str());
        }

        if (Event.type == everward::simulation::DomainEventType::ScanCompleted ||
            Event.type == everward::simulation::DomainEventType::ScanCancelled)
        {
            LastScanLifecycleNotice.Sequence = ++ScanLifecycleSequence;
            LastScanLifecycleNotice.bCompleted =
                Event.type == everward::simulation::DomainEventType::ScanCompleted;
            LastScanLifecycleNotice.bCancelled =
                Event.type == everward::simulation::DomainEventType::ScanCancelled;
            LastScanLifecycleNotice.Detail = UTF8_TO_TCHAR(Event.detail.c_str());
        }
    }

    if (Manipulators != nullptr)
    {
        const auto ManipulatorEvents = Manipulators->drain_events();
        for (const auto& Event : ManipulatorEvents)
        {
            UE_LOG(
                LogTemp,
                Log,
                TEXT("Everward manipulator: %s // %s"),
                Event.arm == everward::simulation::ManipulatorArmId::Port ? TEXT("PORT") : TEXT("STARBOARD"),
                UTF8_TO_TCHAR(Event.detail.c_str()));
        }
    }
}

int64 UProbeSimulationAdapter::GetSimulationTick() const
{
    return Core != nullptr ? Core->tick() : 0;
}

FVector UProbeSimulationAdapter::GetProbePositionMeters() const
{
    if (Core == nullptr) return FVector::ZeroVector;
    const auto& Position = Core->snapshot().position_m;
    return FVector(Position.x, Position.y, Position.z);
}

FEverwardProbeTelemetry UProbeSimulationAdapter::GetProbeTelemetry() const
{
    FEverwardProbeTelemetry Telemetry;
    if (Core == nullptr) return Telemetry;

    const auto& Snapshot = Core->snapshot();
    Telemetry.ProbeId = UTF8_TO_TCHAR(Snapshot.probe_id.c_str());
    Telemetry.Generation = static_cast<int32>(Snapshot.generation);
    Telemetry.SimulationTick = Core->tick();
    Telemetry.SimulationTimeSeconds = static_cast<double>(Telemetry.SimulationTick) / SimulationTicksPerSecond;
    Telemetry.MassKilograms = Snapshot.mass_kg;
    Telemetry.CollisionEnvelopeRadiusMeters = Snapshot.collision_envelope_radius_m;
    Telemetry.bHasContactHistory = Snapshot.has_contact_history;
    Telemetry.LastContactBodyId = UTF8_TO_TCHAR(Snapshot.last_contact_body_id.c_str());
    Telemetry.LastContactPointMeters = FVector(
        Snapshot.last_contact_point_m.x,
        Snapshot.last_contact_point_m.y,
        Snapshot.last_contact_point_m.z);
    Telemetry.LastContactSurfaceNormal = FVector(
        Snapshot.last_contact_surface_normal.x,
        Snapshot.last_contact_surface_normal.y,
        Snapshot.last_contact_surface_normal.z);
    Telemetry.LastContactRelativeVelocityMetersPerSecond = FVector(
        Snapshot.last_contact_relative_velocity_mps.x,
        Snapshot.last_contact_relative_velocity_mps.y,
        Snapshot.last_contact_relative_velocity_mps.z);
    Telemetry.LastContactNormalSpeedMetersPerSecond = Snapshot.last_contact_normal_speed_mps;
    Telemetry.LastContactTick = Snapshot.last_contact_tick;

    const auto& Integrity = Core->component_integrity();
    Telemetry.SensorsIntegrity = Integrity.sensors;
    Telemetry.PropulsionIntegrity = Integrity.propulsion;
    Telemetry.ComputationIntegrity = Integrity.computation;
    Telemetry.ThermalIntegrity = Integrity.thermal;
    if (Core->last_impact().has_value())
    {
        const auto& Impact = *Core->last_impact();
        Telemetry.bHasImpactHistory = true;
        Telemetry.LastImpactEnergyJoules = Impact.impact_energy_j;
        Telemetry.LastImpactSeverity = UTF8_TO_TCHAR(
            everward::simulation::ImpactDamageModel::severity_name(Impact.severity));
        Telemetry.LastImpactSubsystem = SimulationSubsystemName(Impact.affected_subsystem);
        Telemetry.LastImpactIntegrityBefore = Impact.integrity_before;
        Telemetry.LastImpactIntegrityAfter = Impact.integrity_after;
    }

    Telemetry.StoredEnergyJoules = Snapshot.stored_energy_j;
    Telemetry.EnergyCapacityJoules = Snapshot.energy_capacity_j;
    Telemetry.EnergyGenerationWatts = Snapshot.energy_generation_w;
    Telemetry.PowerCapacityWatts = Snapshot.power_capacity_w;
    Telemetry.PowerAllocatedSensorsWatts = Snapshot.power_allocated_sensors_w;
    Telemetry.PowerAllocatedPropulsionWatts = Snapshot.power_allocated_propulsion_w;
    Telemetry.PowerAllocatedComputationWatts = Snapshot.power_allocated_computation_w;
    Telemetry.PowerAllocatedThermalWatts = Snapshot.power_allocated_thermal_w;
    Telemetry.TotalPowerAllocatedWatts = Core->total_power_allocated_w();
    Telemetry.TemperatureKelvin = Snapshot.temperature_k;
    Telemetry.StorageUsedKilograms = Snapshot.storage_used_kg;
    Telemetry.StorageCapacityKilograms = Snapshot.storage_capacity_kg;
    Telemetry.VelocityMetersPerSecond = FVector(
        Snapshot.velocity_mps.x,
        Snapshot.velocity_mps.y,
        Snapshot.velocity_mps.z);
    Telemetry.AttitudeDegrees = FRotator(
        Snapshot.attitude_degrees.pitch,
        Snapshot.attitude_degrees.yaw,
        Snapshot.attitude_degrees.roll);
    Telemetry.bIsScanning = Snapshot.is_scanning;
    Telemetry.ActiveScanTargetId = UTF8_TO_TCHAR(Snapshot.active_scan_target_id.c_str());
    Telemetry.ScanRemainingSeconds = Snapshot.scan_remaining_s;
    Telemetry.bIsOverheated = Snapshot.is_overheated;
    Telemetry.bIsEnergyDepleted = Snapshot.is_energy_depleted;
    return Telemetry;
}

TArray<FEverwardProbeCapability> UProbeSimulationAdapter::GetInstalledCapabilities() const
{
    TArray<FEverwardProbeCapability> Capabilities;
    if (Core == nullptr) return Capabilities;

    const auto& Snapshot = Core->snapshot();
    const FString SharedLockout = SharedProbeLockoutReason(Snapshot);

    auto AddCapability = [&Capabilities](
        FName Id, const TCHAR* Name, const TCHAR* Description,
        bool bOperational, bool bAvailable, bool bSupportsManualControl,
        bool bSupportsAutomation, double AllocatedPowerWatts,
        double MinimumOperatingPowerWatts, double IntegrityFraction,
        FString StatusReason)
    {
        FEverwardProbeCapability Capability;
        Capability.CapabilityId = Id;
        Capability.DisplayName = Name;
        Capability.Description = Description;
        Capability.bInstalled = true;
        Capability.bOperational = bOperational;
        Capability.bAvailable = bAvailable;
        Capability.bSupportsManualControl = bSupportsManualControl;
        Capability.bSupportsAutomation = bSupportsAutomation;
        Capability.AllocatedPowerWatts = AllocatedPowerWatts;
        Capability.MinimumOperatingPowerWatts = MinimumOperatingPowerWatts;
        Capability.IntegrityFraction = IntegrityFraction;
        Capability.StatusReason = MoveTemp(StatusReason);
        Capabilities.Add(MoveTemp(Capability));
    };

    const double PropulsionIntegrity = Core->subsystem_integrity(everward::simulation::PowerSubsystem::Propulsion);
    FString PropulsionReason = TEXT("NOMINAL // COMMAND-DRIVEN PHASE-2 THRUST");
    if (!Snapshot.propulsion_operational) PropulsionReason = TEXT("HARDWARE FAILURE");
    else if (!SharedLockout.IsEmpty()) PropulsionReason = SharedLockout;
    PropulsionReason += TEXT(" // ") + IntegrityReason(*Core, everward::simulation::PowerSubsystem::Propulsion);
    AddCapability(FName(TEXT("propulsion")), TEXT("Propulsion"),
        TEXT("Translation and maneuvering authority."), Snapshot.propulsion_operational,
        Snapshot.can_thrust, true, true, Snapshot.power_allocated_propulsion_w, 0.0,
        PropulsionIntegrity, PropulsionReason);

    const bool bSensorsHaveOperatingPower = Snapshot.power_allocated_sensors_w >=
        everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW;
    const double SensorsIntegrity = Core->subsystem_integrity(everward::simulation::PowerSubsystem::Sensors);
    FString SensorReason = TEXT("NOMINAL");
    if (!Snapshot.sensors_operational) SensorReason = TEXT("HARDWARE FAILURE");
    else if (!SharedLockout.IsEmpty()) SensorReason = SharedLockout;
    else if (!bSensorsHaveOperatingPower)
    {
        SensorReason = FString::Printf(TEXT("BELOW MINIMUM POWER // NEED %.0f W"),
            everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW);
    }
    SensorReason += TEXT(" // ") + IntegrityReason(*Core, everward::simulation::PowerSubsystem::Sensors);
    AddCapability(FName(TEXT("sensors")), TEXT("Sensors"),
        TEXT("Scientific observation and active scanning."), Snapshot.sensors_operational,
        Snapshot.can_scan && bSensorsHaveOperatingPower, true, true,
        Snapshot.power_allocated_sensors_w,
        everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW,
        SensorsIntegrity, SensorReason);

    const bool bComputationHasOperatingPower = Snapshot.power_allocated_computation_w >=
        everward::simulation::ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW;
    const double ComputationIntegrity = Core->subsystem_integrity(everward::simulation::PowerSubsystem::Computation);
    FString ComputationReason = bComputationHasOperatingPower
        ? TEXT("AUTOMATION EXECUTOR READY")
        : FString::Printf(TEXT("BELOW MINIMUM POWER // NEED %.0f W"),
            everward::simulation::ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
    if (!Snapshot.computation_operational) ComputationReason = TEXT("HARDWARE FAILURE");
    ComputationReason += TEXT(" // ") + IntegrityReason(*Core, everward::simulation::PowerSubsystem::Computation);
    AddCapability(FName(TEXT("computation")), TEXT("Computation"),
        TEXT("Onboard planning, automation, and software execution."),
        Snapshot.computation_operational,
        Snapshot.computation_operational && bComputationHasOperatingPower,
        false, true, Snapshot.power_allocated_computation_w,
        everward::simulation::ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW,
        ComputationIntegrity, ComputationReason);

    const double ThermalIntegrity = Core->subsystem_integrity(everward::simulation::PowerSubsystem::Thermal);
    FString ThermalReason = Snapshot.thermal_operational
        ? TEXT("PASSIVE COOLING PATH AVAILABLE") : TEXT("HARDWARE FAILURE");
    if (Snapshot.thermal_operational && Snapshot.is_overheated)
        ThermalReason = TEXT("PROBE OVERHEATED // RECOVERY IN PROGRESS");
    ThermalReason += TEXT(" // ") + IntegrityReason(*Core, everward::simulation::PowerSubsystem::Thermal);
    AddCapability(FName(TEXT("thermal")), TEXT("Thermal Control"),
        TEXT("Heat rejection and thermal-management hardware."),
        Snapshot.thermal_operational, Snapshot.thermal_operational, true, true,
        Snapshot.power_allocated_thermal_w, 0.0, ThermalIntegrity, ThermalReason);

    return Capabilities;
}

FEverwardSoftwarePolicyStatus UProbeSimulationAdapter::GetSoftwarePolicyStatus() const
{
    FEverwardSoftwarePolicyStatus Result;
    if (Core == nullptr) return Result;
    const auto Status = Core->policy_status();
    Result.bInstalled = Status.installed;
    Result.bEnabled = Status.enabled;
    Result.bExecutorAvailable = Status.executor_available;
    Result.PolicyId = UTF8_TO_TCHAR(Status.policy_id.c_str());
    Result.RuleCount = static_cast<int32>(Status.rule_count);
    Result.MinimumComputationPowerWatts = Status.minimum_computation_power_w;
    return Result;
}

TArray<FEverwardManipulatorArmState> UProbeSimulationAdapter::GetManipulatorArmStates() const
{
    TArray<FEverwardManipulatorArmState> Result;
    if (Manipulators == nullptr) return Result;
    Result.Add(ToUnrealManipulatorArmState(
        EEverwardManipulatorArmId::Port, Manipulators->arm(everward::simulation::ManipulatorArmId::Port)));
    Result.Add(ToUnrealManipulatorArmState(
        EEverwardManipulatorArmId::Starboard, Manipulators->arm(everward::simulation::ManipulatorArmId::Starboard)));
    return Result;
}

FEverwardManipulatorReachStatus UProbeSimulationAdapter::GetManipulatorReachStatus(EEverwardManipulatorArmId ArmId) const
{
    // Slice 7 "align a manipulator" minimum interaction: read-only, always
    // recomputed live from Core's authoritative pose/target-selection state
    // and Manipulators' authoritative arm state, never cached, following the
    // same pattern GetSelectedTargetStatus() and GetManipulatorArmStates()
    // already use.
    FEverwardManipulatorReachStatus Result;
    if (Core == nullptr || Manipulators == nullptr) return Result;

    const auto SimulationArmId = ToSimulationManipulatorArmId(ArmId);
    const auto Reach = everward::simulation::manipulator_reach_status(*Core, SimulationArmId, Manipulators->arm(SimulationArmId));
    if (!Reach.has_value()) return Result;

    Result.bHasResult = true;
    Result.bInReach = Reach->in_reach;
    Result.WristRangeToSurfaceMeters = Reach->wrist_range_to_surface_m;
    Result.RemainingDistanceMeters = Reach->remaining_distance_m;
    return Result;
}

FEverwardProbeCommandResult UProbeSimulationAdapter::GetLastCommandResult() const { return LastCommandResult; }
FEverwardAutomationNotice UProbeSimulationAdapter::GetLastAutomationNotice() const { return LastAutomationNotice; }
FEverwardScanLifecycleNotice UProbeSimulationAdapter::GetLastScanLifecycleNotice() const { return LastScanLifecycleNotice; }

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSetVelocityMetersPerSecond(FVector VelocityMetersPerSecond)
{
    const FName CommandId(TEXT("set_velocity"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Core->set_velocity_mps({VelocityMetersPerSecond.X, VelocityMetersPerSecond.Y, VelocityMetersPerSecond.Z});
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("velocity accepted: [%.2f, %.2f, %.2f] m/s"),
                VelocityMetersPerSecond.X, VelocityMetersPerSecond.Y, VelocityMetersPerSecond.Z));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandAdjustLocalVelocityMetersPerSecond(FVector Delta)
{
    const FName CommandId(TEXT("adjust_local_velocity"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Core->adjust_local_velocity_mps({Delta.X, Delta.Y, Delta.Z});
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("local velocity trim accepted: [%.2f, %.2f, %.2f] m/s"), Delta.X, Delta.Y, Delta.Z));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandAdjustAttitudeDegrees(FRotator Delta)
{
    const FName CommandId(TEXT("adjust_attitude"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Core->adjust_attitude_degrees({Delta.Yaw, Delta.Pitch, Delta.Roll});
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("attitude trim accepted: yaw %.1f, pitch %.1f, roll %.1f deg"), Delta.Yaw, Delta.Pitch, Delta.Roll));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandStartScan(const FString& TargetId, double DurationSeconds)
{
    const FName CommandId(TEXT("start_scan"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Core->start_scan(std::string(TCHAR_TO_UTF8(*TargetId)), DurationSeconds);
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("scan started: %s (%.1f s)"), *TargetId, DurationSeconds));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandCancelScan()
{
    const FName CommandId(TEXT("cancel_scan"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Core->cancel_scan();
        return RecordCommandResult(CommandId, true, TEXT("active scan cancelled"));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandAllocatePower(EEverwardPowerSubsystem Subsystem, double Watts)
{
    const FName CommandId(TEXT("allocate_power"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        const bool bWasScanning = Core->snapshot().is_scanning;
        Core->allocate_power(ToSimulationPowerSubsystem(Subsystem), Watts);
        const bool bSensorPowerAbortedScan = Subsystem == EEverwardPowerSubsystem::Sensors &&
            bWasScanning && !Core->snapshot().is_scanning &&
            Watts < everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW;
        return RecordCommandResult(CommandId, true,
            bSensorPowerAbortedScan
                ? FString::Printf(TEXT("sensors power set to %.0f W // active scan aborted below %.0f W minimum"),
                    Watts, everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW)
                : FString::Printf(TEXT("%s power set to %.0f W"), PowerSubsystemName(Subsystem), Watts));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandInstallBasicSurvivalPolicy()
{
    const FName CommandId(TEXT("install_basic_survival_policy"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        everward::simulation::SoftwarePolicy Policy;
        Policy.id = "gen1_basic_survival";
        Policy.rules = {
            {"shed_sensors_below_60_percent_energy", everward::simulation::PolicyConditionKind::EnergyFractionBelow,
             0.60, everward::simulation::PolicyActionKind::SetPowerAllocation,
             everward::simulation::PowerSubsystem::Sensors, 0.0},
            {"shed_propulsion_above_350_kelvin", everward::simulation::PolicyConditionKind::TemperatureAboveKelvin,
             350.0, everward::simulation::PolicyActionKind::SetPowerAllocation,
             everward::simulation::PowerSubsystem::Propulsion, 0.0},
        };
        Core->install_policy(std::move(Policy));
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("GEN1 BASIC SURVIVAL installed; automation requires >= %.0f W computation"),
                everward::simulation::ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandClearSoftwarePolicy()
{
    const FName CommandId(TEXT("clear_software_policy"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    if (!Core->policy_status().installed)
        return RecordCommandResult(CommandId, false, TEXT("no software policy installed"));
    Core->clear_policy();
    return RecordCommandResult(CommandId, true, TEXT("software policy cleared"));
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandDeployManipulatorArm(EEverwardManipulatorArmId ArmId)
{
    const FName CommandId(TEXT("deploy_manipulator_arm"));
    if (Manipulators == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Manipulators->begin_deploy(ToSimulationManipulatorArmId(ArmId));
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("%s arm deploying"), ManipulatorArmName(ArmId)));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandStowManipulatorArm(EEverwardManipulatorArmId ArmId)
{
    const FName CommandId(TEXT("stow_manipulator_arm"));
    if (Manipulators == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Manipulators->begin_stow(ToSimulationManipulatorArmId(ArmId));
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("%s arm stowing"), ManipulatorArmName(ArmId)));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSetManipulatorJointTargetDegrees(
    EEverwardManipulatorArmId ArmId, EEverwardManipulatorJoint Joint, double TargetDegrees)
{
    const FName CommandId(TEXT("set_manipulator_joint_target"));
    if (Manipulators == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Manipulators->command_joint_target_degrees(
            ToSimulationManipulatorArmId(ArmId), ToSimulationManipulatorJoint(Joint), TargetDegrees);
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("%s %s target set to %.1f deg"),
                ManipulatorArmName(ArmId), ManipulatorJointName(Joint), TargetDegrees));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandAttachManipulatorTool(EEverwardManipulatorArmId ArmId)
{
    const FName CommandId(TEXT("attach_manipulator_tool"));
    if (Manipulators == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Manipulators->attach_tool(ToSimulationManipulatorArmId(ArmId));
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("%s arm tool attached"), ManipulatorArmName(ArmId)));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandDetachManipulatorTool(EEverwardManipulatorArmId ArmId)
{
    const FName CommandId(TEXT("detach_manipulator_tool"));
    if (Manipulators == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        Manipulators->detach_tool(ToSimulationManipulatorArmId(ArmId));
        return RecordCommandResult(CommandId, true,
            FString::Printf(TEXT("%s arm tool detached"), ManipulatorArmName(ArmId)));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandGraspSelectedTarget(EEverwardManipulatorArmId ArmId)
{
    // Slice 7 "grasp or dock with a simple object": gated by the exact same
    // reach envelope the manipulator HUD page's REACH row already reports
    // (see attempt_grasp_selected_target), so this never succeeds while that
    // row would read OUT OF REACH and never fails while it would read IN
    // REACH. A false return is an ordinary "not yet" outcome, not an error.
    const FName CommandId(TEXT("grasp_selected_target"));
    if (Core == nullptr || Manipulators == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        const auto SimulationArmId = ToSimulationManipulatorArmId(ArmId);
        const bool bGrasped = everward::simulation::attempt_grasp_selected_target(*Manipulators, *Core, SimulationArmId);
        return RecordCommandResult(CommandId, bGrasped,
            bGrasped
                ? FString::Printf(TEXT("%s arm grasped selected target"), ManipulatorArmName(ArmId))
                : FString::Printf(TEXT("%s arm not in reach of selected target"), ManipulatorArmName(ArmId)));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandReleaseGraspedTarget(EEverwardManipulatorArmId ArmId)
{
    // Slice 7 "release-with-consequence": gated by
    // attempt_release_grasped_target, which fails closed (no mutation, arm
    // keeps holding the target) whenever letting go now would leave the
    // released body overlapping the probe's own hull envelope. A false
    // return is an ordinary "not yet" outcome (move clear of the hull first),
    // not an error.
    const FName CommandId(TEXT("release_grasped_target"));
    if (Core == nullptr || Manipulators == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        const auto SimulationArmId = ToSimulationManipulatorArmId(ArmId);
        const bool bReleased = everward::simulation::attempt_release_grasped_target(*Manipulators, *Core, SimulationArmId);
        return RecordCommandResult(CommandId, bReleased,
            bReleased
                ? FString::Printf(TEXT("%s arm released grasped target"), ManipulatorArmName(ArmId))
                : FString::Printf(TEXT("%s arm cannot release: target would collide with probe hull"), ManipulatorArmName(ArmId)));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSaveGame()
{
    // Wires save_data.hpp's capture/serialize path (already ctest-verified
    // as an exact round trip, see everward_save_data_tests) to a single
    // human-inspectable JSON file. Only the state save_data.hpp's schema
    // covers today is written -- see ProbeSaveData's own header comment for
    // what is intentionally not yet represented.
    const FName CommandId(TEXT("save_game"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    try
    {
        everward::simulation::SaveGameV1 Save;
        Save.simulation_tick = Core->tick();
        Save.probes.push_back(
            Manipulators != nullptr
                ? everward::simulation::capture_probe_save_data(*Core, *Manipulators)
                : everward::simulation::capture_probe_save_data(*Core));

        const FString Json = UTF8_TO_TCHAR(everward::simulation::serialize_save_game(Save).c_str());
        const FString FilePath = SaveGameFilePath();
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
        const bool bWritten =
            FFileHelper::SaveStringToFile(Json, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        return RecordCommandResult(
            CommandId, bWritten, bWritten ? TEXT("probe state saved") : TEXT("failed to write save file"));
    }
    catch (const std::exception& Error) { return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what())); }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandLoadGame()
{
    // Fail-closed by construction order: the new runtime/rig are fully
    // built (validating every field save_data.hpp's restore path checks --
    // unsupported save_version, malformed JSON, out-of-range/inconsistent
    // state, an unreachable manipulator pose) before anything currently
    // live is touched. A rejected load therefore leaves Core/Manipulators
    // completely unchanged rather than partially replaced.
    const FName CommandId(TEXT("load_game"));
    if (Core == nullptr) return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *SaveGameFilePath()))
    {
        return RecordCommandResult(CommandId, false, TEXT("no save file found"));
    }

    everward::simulation::DamageAwareProbeRuntime* NewCore = nullptr;
    everward::simulation::ManipulatorRig* NewManipulators = nullptr;
    try
    {
        const everward::simulation::SaveGameV1 Save =
            everward::simulation::deserialize_save_game(TCHAR_TO_UTF8(*Json));
        if (Save.probes.empty())
        {
            return RecordCommandResult(CommandId, false, TEXT("save file has no probe state"));
        }
        const everward::simulation::ProbeSaveData& Data = Save.probes[0];

        NewCore = new everward::simulation::DamageAwareProbeRuntime(
            everward::simulation::restore_probe_runtime(Data, Save.simulation_tick));
        NewManipulators = new everward::simulation::ManipulatorRig(
            everward::simulation::restore_manipulator_rig(Data, BuildManipulatorCollisionGuard(NewCore)));
    }
    catch (const std::exception& Error)
    {
        delete NewManipulators;
        delete NewCore;
        return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what()));
    }

    delete Manipulators;
    delete Core;
    Core = NewCore;
    Manipulators = NewManipulators;
    SyncOwnerTransformFromSimulation();
    return RecordCommandResult(CommandId, true, TEXT("probe state loaded"));
}

void UProbeSimulationAdapter::SetProbeVelocityMetersPerSecond(FVector VelocityMetersPerSecond)
{
    (void)CommandSetVelocityMetersPerSecond(VelocityMetersPerSecond);
}

FEverwardProbeCommandResult UProbeSimulationAdapter::RecordCommandResult(FName CommandId, bool bAccepted, const FString& Detail)
{
    FEverwardProbeCommandResult Result;
    Result.Sequence = ++CommandSequence;
    Result.CommandId = CommandId;
    Result.bAccepted = bAccepted;
    Result.Detail = Detail;
    LastCommandResult = Result;
    return Result;
}

void UProbeSimulationAdapter::SyncOwnerTransformFromSimulation()
{
    if (Core == nullptr) return;
    AActor* Owner = GetOwner();
    if (Owner == nullptr) return;

    const auto& PositionMeters = Core->snapshot().position_m;
    const FVector PresentationPositionCentimeters(
        PositionMeters.x * MetersToCentimeters,
        PositionMeters.y * MetersToCentimeters,
        PositionMeters.z * MetersToCentimeters);

    const auto& AttitudeDegrees = Core->snapshot().attitude_degrees;
    const FRotator PresentationAttitude(
        AttitudeDegrees.pitch,
        AttitudeDegrees.yaw,
        AttitudeDegrees.roll);

    Owner->SetActorLocation(PresentationPositionCentimeters, false, nullptr, ETeleportType::TeleportPhysics);
    Owner->SetActorRotation(PresentationAttitude, ETeleportType::TeleportPhysics);
}
