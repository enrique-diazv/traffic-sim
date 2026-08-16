#include "trafficsim/core/SimulationClock.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace trafficsim
{

SimulationClock::SimulationClock(double timeStepSeconds) : timeStepSeconds_{timeStepSeconds}
{
    if (!std::isfinite(timeStepSeconds_) || timeStepSeconds_ <= 0.0)
    {
        throw std::invalid_argument{"Simulation clock time step must be finite and positive"};
    }
}

void SimulationClock::advance()
{
    if (tickCount_ == std::numeric_limits<std::uint64_t>::max())
    {
        throw std::overflow_error{"Simulation clock tick count overflow"};
    }

    ++tickCount_;
}

void SimulationClock::reset() noexcept
{
    tickCount_ = 0;
}

double SimulationClock::currentTimeSeconds() const noexcept
{
    return static_cast<double>(tickCount_) * timeStepSeconds_;
}

double SimulationClock::timeStepSeconds() const noexcept
{
    return timeStepSeconds_;
}

std::uint64_t SimulationClock::tickCount() const noexcept
{
    return tickCount_;
}

} // namespace trafficsim
