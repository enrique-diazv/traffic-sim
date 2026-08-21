#include "trafficsim/routing/CongestionAwareRouteCost.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::CongestionAwareRouteCost;
using trafficsim::CongestionCostConfig;
using trafficsim::CongestionState;
using trafficsim::Road;
using trafficsim::RoadProperties;
using trafficsim::RoadTrafficMetrics;

Road createRoad()
{
    return Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 100.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 20,
        },
    };
}

RoadTrafficMetrics createMetrics()
{
    return RoadTrafficMetrics{
        .roadId = 10,
        .vehicleCount = 1,
        .vehiclesPerKilometer = 10.0,
        .averageSpeedMetersPerSecond = 10.0,
        .occupancy = 0.5,
        .speedRatio = 0.5,
        .congestionState = CongestionState::FreeFlow,
    };
}

TEST(CongestionAwareRouteCostTests, CalculatesFreeFlowTravelTime)
{
    const CongestionAwareRouteCost routeCost;

    EXPECT_DOUBLE_EQ(routeCost.calculate(createRoad()), 5.0);
}

TEST(CongestionAwareRouteCostTests, EmptyRoadUsesFreeFlowCost)
{
    const CongestionAwareRouteCost routeCost;
    auto metrics = createMetrics();

    metrics.vehicleCount = 0;
    metrics.averageSpeedMetersPerSecond = 0.0;
    metrics.speedRatio = 0.0;

    EXPECT_DOUBLE_EQ(routeCost.calculate(createRoad(), metrics), 5.0);
}

TEST(CongestionAwareRouteCostTests, IncreasesCostWithCongestionSeverity)
{
    const CongestionAwareRouteCost routeCost;
    const auto road = createRoad();
    auto metrics = createMetrics();

    EXPECT_DOUBLE_EQ(routeCost.calculate(road, metrics), 10.0);

    metrics.congestionState = CongestionState::Moderate;
    EXPECT_DOUBLE_EQ(routeCost.calculate(road, metrics), 11.0);

    metrics.congestionState = CongestionState::Congested;
    EXPECT_DOUBLE_EQ(routeCost.calculate(road, metrics), 15.0);

    metrics.congestionState = CongestionState::Gridlock;
    EXPECT_DOUBLE_EQ(routeCost.calculate(road, metrics), 30.0);
}

TEST(CongestionAwareRouteCostTests, AppliesMinimumSpeedRatio)
{
    const CongestionAwareRouteCost routeCost;
    auto metrics = createMetrics();

    metrics.speedRatio = 0.0;
    metrics.congestionState = CongestionState::Gridlock;

    EXPECT_DOUBLE_EQ(routeCost.calculate(createRoad(), metrics), 150.0);
}

TEST(CongestionAwareRouteCostTests, RejectsInvalidConfigurationAndMetrics)
{
    auto config = CongestionCostConfig{};
    config.minimumSpeedRatio = 0.0;

    EXPECT_THROW(static_cast<void>(CongestionAwareRouteCost{config}), std::invalid_argument);

    config = CongestionCostConfig{};
    config.congestedPenaltyMultiplier = 1.0;

    EXPECT_THROW(static_cast<void>(CongestionAwareRouteCost{config}), std::invalid_argument);

    const CongestionAwareRouteCost routeCost;
    const auto road = createRoad();
    auto metrics = createMetrics();

    metrics.roadId = 99;
    EXPECT_THROW(static_cast<void>(routeCost.calculate(road, metrics)), std::invalid_argument);

    metrics.roadId = 10;
    metrics.speedRatio = std::numeric_limits<double>::infinity();
    EXPECT_THROW(static_cast<void>(routeCost.calculate(road, metrics)), std::invalid_argument);
}

} // namespace