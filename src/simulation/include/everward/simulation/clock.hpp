#pragma once

#include <cstdint>
#include <stdexcept>

namespace everward::simulation {

class SimulationClock {
public:
    static constexpr std::int64_t TicksPerSecond = 1'000'000;

    explicit SimulationClock(std::int64_t initial_tick = 0) : tick_(initial_tick) {
        if (initial_tick < 0) {
            throw std::invalid_argument("initial_tick must be non-negative");
        }
    }

    [[nodiscard]] std::int64_t tick() const noexcept { return tick_; }

    void advance_by(std::int64_t delta_ticks) {
        if (delta_ticks < 0) {
            throw std::invalid_argument("delta_ticks must be non-negative");
        }
        tick_ += delta_ticks;
    }

private:
    std::int64_t tick_;
};

} // namespace everward::simulation
