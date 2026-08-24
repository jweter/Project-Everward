#include "ProbeSimulationAdapter.h"

#include "GameFramework/Actor.h"
#include "everward/simulation/core.hpp"

#include <exception>
#include <string>

namespace
{
everward::simulation::PowerSubsystem ToSimulationPowerSubsystem(EEverwardPowerSubsystem Subsystem)
{
    switch (Subsystem)
    {
        case EEverwardPowerSubsystem::Sensors:
            return everward::simulation::PowerSubsystem::Sensors;
        case EEverwardPowerSubsystem::Propulsion:
            return everward::simulation::PowerSubsystem::Propulsion;
        case EEverwardPowerSubsystem::Computation:
            return everward::simulation::PowerSubsystem::Computation;
        case EEverwardPowerSubsystem::Thermal:
            return everward::simulation::PowerSubsystem::Thermal;
    }

    return everward::simulation::PowerSubsystem::Sensors;
}

const TCHAR* PowerSubsystemName(EEverwardPowerSubsystem Subsystem)
{
    switch (Subsystem)
    {
        case EEverwardPowerSubsystem::Sensors:
            return TEXT("sensors");
        case EEverwardPowerSubsystem::Propulsion:
            return TEXT("propulsion");
        case EEverwardPowerSubsystem::Computation:
            return TEXT("computation");
        case EEverwardPowerSubsystem::Thermal:
            return TEXT("thermal");
    }

    return TEXT("unknown");
}
}

UProbeSimulationAdapter::UProbeSimulationAdapter()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UProbeSimulationAdapter::BeginPlay()
{
    Super::BeginPlay();
    Core = new everward::simulation::SimulationCore(
        everward::simulation::SimulationCore::make_canonical_ev0001());
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

    if (Core == nullptr)
    {
        return;
    }

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
    }
}

int64 UProbeSimulationAdapter::GetSimulationTick() const
{
    return Core != nullptr ? Core->tick() : 0;
}

FVector UProbeSimulationAdapter::GetProbePositionMeters() const
{
    if (Core == nullptr)
    {
        return FVector::ZeroVector;
    }

    const auto& Position = Core->snapshot().position_m;
    return FVector(Position.x, Position.y, Position.z);
}

FEverwardProbeTelemetry UProbeSimulationAdapter::GetProbeTelemetry() const
{
    FEverwardProbeTelemetry Telemetry;
    if (Core == nullptr)
    {
        return Telemetry;
    }

    const auto& Snapshot = Core->snapshot();
    Telemetry.ProbeId = UTF8_TO_TCHAR(Snapshot.probe_id.c_str());
    Telemetry.Generation = static_cast<int32>(Snapshot.generation);
    Telemetry.SimulationTick = Core->tick();
    Telemetry.SimulationTimeSeconds = static_cast<double>(Telemetry.SimulationTick) / SimulationTicksPerSecond;
    Telemetry.MassKilograms = Snapshot.mass_kg;
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
    if (Core == nullptr)
    {
        return Capabilities;
    }

    const auto& Snapshot = Core->snapshot();

    auto AddCapability = [&Capabilities](
        FName Id,
        const TCHAR* Name,
        const TCHAR* Description,
        bool bOperational,
        bool bAvailable,
        bool bSupportsManualControl,
        bool bSupportsAutomation,
        double AllocatedPowerWatts)
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
        Capabilities.Add(MoveTemp(Capability));
    };

    AddCapability(
        FName(TEXT("propulsion")),
        TEXT("Propulsion"),
        TEXT("Translation and maneuvering authority."),
        Snapshot.propulsion_operational,
        Snapshot.can_thrust,
        true,
        true,
        Snapshot.power_allocated_propulsion_w);

    AddCapability(
        FName(TEXT("sensors")),
        TEXT("Sensors"),
        TEXT("Scientific observation and active scanning."),
        Snapshot.sensors_operational,
        Snapshot.can_scan,
        true,
        true,
        Snapshot.power_allocated_sensors_w);

    AddCapability(
        FName(TEXT("computation")),
        TEXT("Computation"),
        TEXT("Onboard planning, automation, and software execution."),
        Snapshot.computation_operational,
        Snapshot.computation_operational,
        false,
        true,
        Snapshot.power_allocated_computation_w);

    AddCapability(
        FName(TEXT("thermal")),
        TEXT("Thermal Control"),
        TEXT("Heat rejection and thermal-management hardware."),
        Snapshot.thermal_operational,
        Snapshot.thermal_operational,
        true,
        true,
        Snapshot.power_allocated_thermal_w);

    return Capabilities;
}

FEverwardProbeCommandResult UProbeSimulationAdapter::GetLastCommandResult() const
{
    return LastCommandResult;
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandSetVelocityMetersPerSecond(FVector VelocityMetersPerSecond)
{
    const FName CommandId(TEXT("set_velocity"));
    if (Core == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    }

    try
    {
        Core->set_velocity_mps({
            VelocityMetersPerSecond.X,
            VelocityMetersPerSecond.Y,
            VelocityMetersPerSecond.Z
        });
        return RecordCommandResult(
            CommandId,
            true,
            FString::Printf(
                TEXT("velocity accepted: [%.2f, %.2f, %.2f] m/s"),
                VelocityMetersPerSecond.X,
                VelocityMetersPerSecond.Y,
                VelocityMetersPerSecond.Z));
    }
    catch (const std::exception& Error)
    {
        return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what()));
    }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandStartScan(const FString& TargetId, double DurationSeconds)
{
    const FName CommandId(TEXT("start_scan"));
    if (Core == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    }

    try
    {
        const std::string TargetUtf8(TCHAR_TO_UTF8(*TargetId));
        Core->start_scan(TargetUtf8, DurationSeconds);
        return RecordCommandResult(
            CommandId,
            true,
            FString::Printf(TEXT("scan started: %s (%.1f s)"), *TargetId, DurationSeconds));
    }
    catch (const std::exception& Error)
    {
        return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what()));
    }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandCancelScan()
{
    const FName CommandId(TEXT("cancel_scan"));
    if (Core == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    }

    try
    {
        Core->cancel_scan();
        return RecordCommandResult(CommandId, true, TEXT("active scan cancelled"));
    }
    catch (const std::exception& Error)
    {
        return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what()));
    }
}

FEverwardProbeCommandResult UProbeSimulationAdapter::CommandAllocatePower(
    EEverwardPowerSubsystem Subsystem,
    double Watts)
{
    const FName CommandId(TEXT("allocate_power"));
    if (Core == nullptr)
    {
        return RecordCommandResult(CommandId, false, TEXT("simulation unavailable"));
    }

    try
    {
        Core->allocate_power(ToSimulationPowerSubsystem(Subsystem), Watts);
        return RecordCommandResult(
            CommandId,
            true,
            FString::Printf(TEXT("%s power set to %.0f W"), PowerSubsystemName(Subsystem), Watts));
    }
    catch (const std::exception& Error)
    {
        return RecordCommandResult(CommandId, false, UTF8_TO_TCHAR(Error.what()));
    }
}

void UProbeSimulationAdapter::SetProbeVelocityMetersPerSecond(FVector VelocityMetersPerSecond)
{
    (void)CommandSetVelocityMetersPerSecond(VelocityMetersPerSecond);
}

FEverwardProbeCommandResult UProbeSimulationAdapter::RecordCommandResult(
    FName CommandId,
    bool bAccepted,
    const FString& Detail)
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
    if (Core == nullptr)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return;
    }

    const auto& PositionMeters = Core->snapshot().position_m;
    const FVector PresentationPositionCentimeters(
        PositionMeters.x * MetersToCentimeters,
        PositionMeters.y * MetersToCentimeters,
        PositionMeters.z * MetersToCentimeters);

    Owner->SetActorLocation(PresentationPositionCentimeters, false, nullptr, ETeleportType::TeleportPhysics);
}
