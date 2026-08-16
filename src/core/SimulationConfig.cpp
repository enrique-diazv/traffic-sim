#include "trafficsim/core/SimulationConfig.h"

#include <cmath>
#include <stdexcept>

namespace trafficsim
{

void SimulationConfig::validate() const
{
    if (!std::isfinite(durationSeconds) || durationSeconds <= 0.0)
    {
        throw std::invalid_argument{"Simulation duration must be finite and positive"};
    }

    if (!std::isfinite(timeStepSeconds) || timeStepSeconds <= 0.0)
    {
        throw std::invalid_argument{"Simulation time step must be finite and positive"};
    }

    if (timeStepSeconds > durationSeconds)
    {
        throw std::invalid_argument{"Simulation time step cannot exceed total duration"};
    }

    if (maximumVehicles == 0)
    {
        throw std::invalid_argument{"Simulation maximum vehicles must be greater than zero"};
    }

    defaultVehicleDynamics.validate();

    if (!std::isfinite(minimumFollowingDistanceMeters) || minimumFollowingDistanceMeters < 0.0)
    {
        throw std::invalid_argument{"Minimum following distance must be finite and non-negative"};
    }

    if (!std::isfinite(reactionTimeSeconds) || reactionTimeSeconds <= 0.0)
    {
        throw std::invalid_argument{"Reaction time must be finite and positive"};
    }
}

} // namespace trafficsim
