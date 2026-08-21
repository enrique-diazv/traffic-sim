#ifndef TRAFFICSIM_TRAFFIC_ROAD_TRAFFIC_METRICS_H
#define TRAFFICSIM_TRAFFIC_ROAD_TRAFFIC_METRICS_H

#include "trafficsim/network/Types.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace trafficsim
{

enum class CongestionState : std::uint8_t
{
    FreeFlow,
    Moderate,
    Congested,
    Gridlock,
};

[[nodiscard]] constexpr std::string_view congestionStateName(CongestionState state) noexcept
{
    switch (state)
    {
    case CongestionState::FreeFlow:
        return "free_flow";
    case CongestionState::Moderate:
        return "moderate";
    case CongestionState::Congested:
        return "congested";
    case CongestionState::Gridlock:
        return "gridlock";
    }

    return "unknown";
}

struct RoadTrafficObservation
{
    std::size_t vehicleCount{};
    double totalSpeedMetersPerSecond{};
};

struct RoadTrafficMetrics
{
    RoadId roadId{};
    std::size_t vehicleCount{};
    double vehiclesPerKilometer{};
    double averageSpeedMetersPerSecond{};
    double occupancy{};
    double speedRatio{};
    CongestionState congestionState{CongestionState::FreeFlow};
};

} // namespace trafficsim

#endif // TRAFFICSIM_TRAFFIC_ROAD_TRAFFIC_METRICS_H