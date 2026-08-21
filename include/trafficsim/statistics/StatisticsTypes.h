#ifndef TRAFFICSIM_STATISTICS_STATISTICS_TYPES_H
#define TRAFFICSIM_STATISTICS_STATISTICS_TYPES_H

#include "trafficsim/network/Types.h"
#include "trafficsim/traffic/RoadTrafficMetrics.h"
#include "trafficsim/vehicles/VehicleTypes.h"

#include <cstddef>

namespace trafficsim
{

struct VehicleResult
{
    VehicleId vehicleId{};
    IntersectionId origin{};
    IntersectionId destination{};
    double spawnTimeSeconds{};
    double arrivalTimeSeconds{};
    double travelTimeSeconds{};
    double waitingTimeSeconds{};
    double distanceMeters{};
    double averageSpeedMetersPerSecond{};
};

struct RoadResult
{
    RoadId roadId{};
    double averageSpeedMetersPerSecond{};
    std::size_t peakVehicleCount{};
    double averageOccupancy{};
    double congestionTimeSeconds{};
    CongestionState peakCongestionState{CongestionState::FreeFlow};
};

struct SimulationSummary
{
    std::size_t vehiclesSpawned{};
    std::size_t vehiclesArrived{};
    double averageTravelTimeSeconds{};
    double minimumTravelTimeSeconds{};
    double maximumTravelTimeSeconds{};
    double averageWaitingTimeSeconds{};
    double averageSpeedMetersPerSecond{};
    double totalDistanceMeters{};
    double averageRouteLengthMeters{};
    std::size_t peakActiveVehicles{};
};

} // namespace trafficsim

#endif // TRAFFICSIM_STATISTICS_STATISTICS_TYPES_H