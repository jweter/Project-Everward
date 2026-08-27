#include "ProbeSimulationAdapter.h"

#include "EverwardPhase2TestEnvironment.h"
#include "GameFramework/Actor.h"
#include "everward/simulation/software_policy.hpp"

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

FString SharedProbeLockoutReason(const everward::simulation::ProbeStateSnapshot& Snapshot)
{
    if (Snapshot.is_energy_depleted) return TEXT("ENERGY DEPLETED");
    if (Snapshot.is_overheated) return TEXT("THERMAL LOCKOUT");
    return FString();
}
}

UProbeSimulationAdapter::UProbeSimulationAdapter()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UProbeSimulationAdapter::BeginPlay()
{
    Super::BeginPlay();
    Core = new everward::simulation::ProbeRuntime(
        everward::simulation::ProbeRuntime::make_canonical_ev0001());

    // Register the same sphere rendered by the temporary Phase-2 environment.
    // The runtime, not Unreal collision response, decides whether EV-0001 can
    // pass through it.
    Core->add_static_sphere_body({
        std::string(TCHAR_TO_UTF8(AEverwardPhase2TestEnvironment::BootstrapScanTargetId)),
        {
            AEverwardPhase2TestEnvironment::BootstrapBodyCenterXMeters,
            AEverwardPhase2TestEnvironment::BootstrapBodyCenterYMeters,
            AEverwardPhase2TestEnvironment::BootstrapBodyCenterZMeters,
        },
        AEverwardPhase2TestEnvironment::BootstrapBodyRadiusMeters,
    });

    SyncOwnerTransformFromSimulation();
}

void UProbeSimulationAdapter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    delete Core;
    Core = nullptr;
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
        FixedStepAccumulatorSeconds -= FixedStepSeconds;
    }

    SyncOwnerTransformFromSimulation();

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
        double MinimumOperatingPowerWatts, FString StatusReason)
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
        Capability.StatusReason = MoveTemp(StatusReason);
        Capabilities.Add(MoveTemp(Capability));
    };

    FString PropulsionReason = TEXT("NOMINAL // COMMAND-DRIVEN PHASE-2 THRUST");
    if (!Snapshot.propulsion_operational) PropulsionReason = TEXT("HARDWARE FAILURE");
    else if (!SharedLockout.IsEmpty()) PropulsionReason = SharedLockout;
    AddCapability(FName(TEXT("propulsion")), TEXT("Propulsion"),
        TEXT("Translation and maneuvering authority."), Snapshot.propulsion_operational,
        Snapshot.can_thrust, true, true, Snapshot.power_allocated_propulsion_w, 0.0,
        PropulsionReason);

    const bool bSensorsHaveOperatingPower = Snapshot.power_allocated_sensors_w >=
        everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW;
    FString SensorReason = TEXT("NOMINAL");
    if (!Snapshot.sensors_operational) SensorReason = TEXT("HARDWARE FAILURE");
    else if (!SharedLockout.IsEmpty()) SensorReason = SharedLockout;
    else if (!bSensorsHaveOperatingPower)
    {
        SensorReason = FString::Printf(TEXT("BELOW MINIMUM POWER // NEED %.0f W"),
            everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW);
    }
    AddCapability(FName(TEXT("sensors")), TEXT("Sensors"),
        TEXT("Scientific observation and active scanning."), Snapshot.sensors_operational,
        Snapshot.can_scan && bSensorsHaveOperatingPower, true, true,
        Snapshot.power_allocated_sensors_w,
        everward::simulation::ProbeRuntime::kGeneration1MinimumSensorPowerW,
        SensorReason);

    const bool bComputationHasOperatingPower = Snapshot.power_allocated_computation_w >=
        everward::simulation::ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW;
    FString ComputationReason = bComputationHasOperatingPower
        ? TEXT("AUTOMATION EXECUTOR READY")
        : FString::Printf(TEXT("BELOW MINIMUM POWER // NEED %.0f W"),
            everward::simulation::ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW);
    if (!Snapshot.computation_operational) ComputationReason = TEXT("HARDWARE FAILURE");
    AddCapability(FName(TEXT("computation")), TEXT("Computation"),
        TEXT("Onboard planning, automation, and software execution."),
        Snapshot.computation_operational,
        Snapshot.computation_operational && bComputationHasOperatingPower,
        false, true, Snapshot.power_allocated_computation_w,
        everward::simulation::ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW,
        ComputationReason);

    FString ThermalReason = Snapshot.thermal_operational
        ? TEXT("PASSIVE COOLING PATH AVAILABLE") : TEXT("HARDWARE FAILURE");
    if (Snapshot.thermal_operational && Snapshot.is_overheated)
        ThermalReason = TEXT("PROBE OVERHEATED // RECOVERY IN PROGRESS");
    AddCapability(FName(TEXT("thermal")), TEXT("Thermal Control"),
        TEXT("Heat rejection and thermal-management hardware."),
        Snapshot.thermal_operational, Snapshot.thermal_operational, true, true,
        Snapshot.power_allocated_thermal_w, 0.0, ThermalReason);

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
