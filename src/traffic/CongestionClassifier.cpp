#include "trafficsim/traffic/CongestionClassifier.h"

#include <cmath>
#include <stdexcept>

namespace trafficsim
{

namespace
{

constexpr double freeFlowOccupancyThreshold{0.25};
constexpr double moderateOccupancyThreshold{0.50};
constexpr double congestedOccupancyThreshold{0.75};
constexpr double fullOccupancyThreshold{1.0};

constexpr double congestedSpeedRatioThreshold{0.50};
constexpr double gridlockSpeedRatioThreshold{0.10};

CongestionState classify(const RoadTrafficMetrics &metrics) noexcept
{
    if (metrics.vehicleCount == 0 || metrics.occupancy < freeFlowOccupancyThreshold)
    {
        return CongestionState::FreeFlow;
    }

    if (metrics.occupancy < moderateOccupancyThreshold)
    {
        return CongestionState::Moderate;
    }

    if (metrics.occupancy < congestedOccupancyThreshold)
    {
        return metrics.speedRatio < congestedSpeedRatioThreshold ? CongestionState::Congested
                                                                 : CongestionState::Moderate;
    }

    if (metrics.occupancy < fullOccupancyThreshold)
    {
        return metrics.speedRatio < gridlockSpeedRatioThreshold ? CongestionState::Gridlock
                                                                : CongestionState::Congested;
    }

    return CongestionState::Gridlock;
}

} // namespace

RoadTrafficMetrics CongestionClassifier::evaluate(const Road &road,
                                                  RoadTrafficObservation observation)
{
    if (!std::isfinite(observation.totalSpeedMetersPerSecond) ||
        observation.totalSpeedMetersPerSecond < 0.0)
    {
        throw std::invalid_argument{"Road total vehicle speed must be finite and non-negative"};
    }

    if (observation.vehicleCount == 0 && observation.totalSpeedMetersPerSecond != 0.0)
    {
        throw std::invalid_argument{"An empty road cannot have a non-zero total vehicle speed"};
    }

    const auto vehicleCount = static_cast<double>(observation.vehicleCount);
    const auto averageSpeed =
        observation.vehicleCount == 0 ? 0.0 : observation.totalSpeedMetersPerSecond / vehicleCount;

    RoadTrafficMetrics metrics{
        .roadId = road.id(),
        .vehicleCount = observation.vehicleCount,
        .vehiclesPerKilometer = vehicleCount * 1000.0 / road.lengthMeters(),
        .averageSpeedMetersPerSecond = averageSpeed,
        .occupancy = vehicleCount / static_cast<double>(road.capacity()),
        .speedRatio = averageSpeed / road.speedLimitMetersPerSecond(),
    };

    if (!std::isfinite(metrics.vehiclesPerKilometer) ||
        !std::isfinite(metrics.averageSpeedMetersPerSecond) || !std::isfinite(metrics.occupancy) ||
        !std::isfinite(metrics.speedRatio))
    {
        throw std::overflow_error{"Road traffic metric overflow"};
    }

    metrics.congestionState = classify(metrics);
    return metrics;
}

} // namespace trafficsim