#include "ProbeSimulationAdapter.h"

#include "GameFramework/Actor.h"
#include "everward/simulation/core.hpp"

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
    Telemetry.SimulationTick = Core->tick();
    Telemetry.SimulationTimeSeconds = static_cast<double>(Telemetry.SimulationTick) / SimulationTicksPerSecond;
    Telemetry.MassKilograms = Snapshot.mass_kg;
    Telemetry.StoredEnergyJoules = Snapshot.stored_energy_j;
    Telemetry.EnergyCapacityJoules = Snapshot.energy_capacity_j;
    Telemetry.TemperatureKelvin = Snapshot.temperature_k;
    Telemetry.StorageUsedKilograms = Snapshot.storage_used_kg;
    Telemetry.StorageCapacityKilograms = Snapshot.storage_capacity_kg;
    Telemetry.VelocityMetersPerSecond = FVector(
        Snapshot.velocity_mps.x,
        Snapshot.velocity_mps.y,
        Snapshot.velocity_mps.z);
    Telemetry.bIsOverheated = Snapshot.is_overheated;
    Telemetry.bIsEnergyDepleted = Snapshot.is_energy_depleted;
    return Telemetry;
}

void UProbeSimulationAdapter::SetProbeVelocityMetersPerSecond(FVector VelocityMetersPerSecond)
{
    if (Core == nullptr)
    {
        return;
    }

    Core->set_velocity_mps({
        VelocityMetersPerSecond.X,
        VelocityMetersPerSecond.Y,
        VelocityMetersPerSecond.Z
    });
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
