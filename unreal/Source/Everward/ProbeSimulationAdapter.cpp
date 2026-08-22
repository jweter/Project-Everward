#include "ProbeSimulationAdapter.h"

#include "everward/simulation/core.hpp"

UProbeSimulationAdapter::UProbeSimulationAdapter()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UProbeSimulationAdapter::BeginPlay()
{
    Super::BeginPlay();
    Core = new everward::simulation::SimulationCore();
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
