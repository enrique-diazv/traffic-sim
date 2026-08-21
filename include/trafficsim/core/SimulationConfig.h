#ifndef TRAFFICSIM_CORE_SIMULATION_CONFIG_H
#define TRAFFICSIM_CORE_SIMULATION_CONFIG_H

#include "trafficsim/routing/ReroutingPolicy.h"
#include "trafficsim/vehicles/VehicleTypes.h"

#include <cstddef>
#include <cstdint>

namespace trafficsim
{

struct SimulationConfig
{
    double durationSeconds{60.0};
    double timeStepSeconds{0.1};
    std::uint64_t randomSeed{42};
    std::size_t maximumVehicles{1000};

    VehicleDynamics defaultVehicleDynamics{
        .maximumSpeedMetersPerSecond = 13.9,
        .accelerationMetersPerSecondSquared = 2.0,
        .decelerationMetersPerSecondSquared = 4.0,
    };

    double minimumFollowingDistanceMeters{2.0};
    double reactionTimeSeconds{1.0};

    ReroutingConfig rerouting{};
    CongestionCostConfig congestionCost{};

    void validate() const;
};

} // namespace trafficsim

#endif // TRAFFICSIM_CORE_SIMULATION_CONFIG_H
