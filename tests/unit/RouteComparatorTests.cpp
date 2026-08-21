#include "trafficsim/routing/RouteComparator.h"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>

namespace
{

using trafficsim::CongestionState;
using trafficsim::Intersection;
using trafficsim::IntersectionId;
using trafficsim::Road;
using trafficsim::RoadId;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::RoadTrafficMetrics;
using trafficsim::RouteComparator;

void addRoad(RoadNetwork &network, RoadId roadId, IntersectionId origin, IntersectionId destination,
             double lengthMeters, double speedLimitMetersPerSecond)
{
    network.addRoad(Road{
        roadId,
        RoadProperties{
            .origin = origin,
            .destination = destination,
            .lengthMeters = lengthMeters,
            .speedLimitMetersPerSecond = speedLimitMetersPerSecond,
            .laneCount = 1,
            .capacity = 20,
        },
    });
}

RoadNetwork createNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {.x = 0.0, .y = 0.0}});
    network.addIntersection(Intersection{2, {.x = 100.0, .y = 0.0}});
    network.addIntersection(Intersection{3, {.x = 100.0, .y = 100.0}});
    network.addIntersection(Intersection{4, {.x = 200.0, .y = 0.0}});

    addRoad(network, 10, 1, 2, 100.0, 10.0);
    addRoad(network, 20, 2, 4, 100.0, 10.0);
    addRoad(network, 30, 1, 3, 120.0, 20.0);
    addRoad(network, 40, 3, 4, 120.0, 20.0);

    return network;
}

TEST(RouteComparatorTests, ComparesFreeFlowRouteCosts)
{
    const auto network = createNetwork();
    const RouteComparator comparator;
    const std::array<RoadId, 2> currentRoute{10, 20};
    const std::array<RoadId, 2> candidateRoute{30, 40};

    const auto result = comparator.compare(network, currentRoute, candidateRoute);

    EXPECT_DOUBLE_EQ(result.currentCost, 20.0);
    EXPECT_DOUBLE_EQ(result.candidateCost, 12.0);
    EXPECT_NEAR(result.relativeImprovement, 0.4, 1.0e-12);
    EXPECT_TRUE(result.candidateIsBetter);
}

TEST(RouteComparatorTests, CongestionCanReverseComparison)
{
    const auto network = createNetwork();
    const RouteComparator comparator;
    const std::array<RoadId, 2> currentRoute{10, 20};
    const std::array<RoadId, 2> candidateRoute{30, 40};
    const std::array metrics{
        RoadTrafficMetrics{
            .roadId = 30,
            .vehicleCount = 20,
            .vehiclesPerKilometer = 166.0,
            .averageSpeedMetersPerSecond = 4.0,
            .occupancy = 1.0,
            .speedRatio = 0.2,
            .congestionState = CongestionState::Gridlock,
        },
    };

    const auto result = comparator.compare(network, currentRoute, candidateRoute, metrics);

    EXPECT_DOUBLE_EQ(result.currentCost, 20.0);
    EXPECT_DOUBLE_EQ(result.candidateCost, 96.0);
    EXPECT_NEAR(result.relativeImprovement, -3.8, 1.0e-12);
    EXPECT_FALSE(result.candidateIsBetter);
}

TEST(RouteComparatorTests, ComparesEmptyRoutes)
{
    const auto network = createNetwork();
    const RouteComparator comparator;
    const std::array<RoadId, 0> emptyRoute{};

    const auto result = comparator.compare(network, emptyRoute, emptyRoute);

    EXPECT_DOUBLE_EQ(result.currentCost, 0.0);
    EXPECT_DOUBLE_EQ(result.candidateCost, 0.0);
    EXPECT_DOUBLE_EQ(result.relativeImprovement, 0.0);
    EXPECT_FALSE(result.candidateIsBetter);
}

TEST(RouteComparatorTests, RejectsMissingAndDisconnectedRoads)
{
    const auto network = createNetwork();
    const RouteComparator comparator;
    const std::array<RoadId, 1> missingRoad{999};
    const std::array<RoadId, 2> disconnectedRoads{20, 10};

    EXPECT_THROW(static_cast<void>(comparator.calculateCost(network, missingRoad)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(comparator.calculateCost(network, disconnectedRoads)),
                 std::invalid_argument);
}

} // namespace