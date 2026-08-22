#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProbeSimulationAdapter.generated.h"

namespace everward::simulation
{
class SimulationCore;
}

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

    UFUNCTION(BlueprintCallable, Category="Everward|Simulation")
    void SetProbeVelocityMetersPerSecond(FVector VelocityMetersPerSecond);

private:
    static constexpr int64 FixedStepTicks = 16667;
    static constexpr double FixedStepSeconds = static_cast<double>(FixedStepTicks) / 1000000.0;

    double FixedStepAccumulatorSeconds = 0.0;
    everward::simulation::SimulationCore* Core = nullptr;
};
