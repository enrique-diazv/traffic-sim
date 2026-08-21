#include "trafficsim/routing/AStarRoutePlanner.h"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>

namespace
{

using trafficsim::AStarRoutePlanner;
using trafficsim::CongestionState;
using trafficsim::Intersection;
using trafficsim::IntersectionId;
using trafficsim::Road;
using trafficsim::RoadId;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::RoadTrafficMetrics;

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

RoadNetwork createRoutingNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {.x = 0.0, .y = 0.0}});
    network.addIntersection(Intersection{2, {.x = 100.0, .y = 0.0}});
    network.addIntersection(Intersection{3, {.x = 100.0, .y = 100.0}});
    network.addIntersection(Intersection{4, {.x = 200.0, .y = 0.0}});
    network.addIntersection(Intersection{5, {.x = 300.0, .y = 0.0}});

    addRoad(network, 10, 1, 2, 100.0, 10.0);
    addRoad(network, 20, 2, 4, 100.0, 10.0);
    addRoad(network, 30, 1, 3, 120.0, 20.0);
    addRoad(network, 40, 3, 4, 120.0, 20.0);

    return network;
}

RoadTrafficMetrics gridlockedMetrics(RoadId roadId)
{
    return RoadTrafficMetrics{
        .roadId = roadId,
        .vehicleCount = 20,
        .vehiclesPerKilometer = 166.0,
        .averageSpeedMetersPerSecond = 4.0,
        .occupancy = 1.0,
        .speedRatio = 0.2,
        .congestionState = CongestionState::Gridlock,
    };
}

TEST(AStarRoutePlannerTests, SelectsFastestFreeFlowRoute)
{
    const auto network = createRoutingNetwork();
    const AStarRoutePlanner planner;

    const auto route = planner.calculateRoute(network, 1, 4);

    ASSERT_TRUE(route.has_value());
    ASSERT_EQ(route->segmentCount(), 2U);
    EXPECT_EQ(route->segments()[0], 30U);
    EXPECT_EQ(route->segments()[1], 40U);
    EXPECT_DOUBLE_EQ(route->totalDistanceMeters(), 240.0);
}

TEST(AStarRoutePlannerTests, AvoidsCongestedRoute)
{
    const auto network = createRoutingNetwork();
    const AStarRoutePlanner planner;
    const std::array metrics{
        gridlockedMetrics(30),
    };

    const auto route = planner.calculateRoute(network, 1, 4, metrics);

    ASSERT_TRUE(route.has_value());
    ASSERT_EQ(route->segmentCount(), 2U);
    EXPECT_EQ(route->segments()[0], 10U);
    EXPECT_EQ(route->segments()[1], 20U);
    EXPECT_DOUBLE_EQ(route->totalDistanceMeters(), 200.0);
}

TEST(AStarRoutePlannerTests, HandlesEmptyAndUnreachableRoutes)
{
    const auto network = createRoutingNetwork();
    const AStarRoutePlanner planner;

    const auto emptyRoute = planner.calculateRoute(network, 1, 1);
    const auto unreachableRoute = planner.calculateRoute(network, 1, 5);

    ASSERT_TRUE(emptyRoute.has_value());
    EXPECT_TRUE(emptyRoute->isComplete());
    EXPECT_DOUBLE_EQ(emptyRoute->totalDistanceMeters(), 0.0);
    EXPECT_FALSE(unreachableRoute.has_value());
}

TEST(AStarRoutePlannerTests, RejectsInvalidEndpointsAndMetrics)
{
    const auto network = createRoutingNetwork();
    const AStarRoutePlanner planner;

    EXPECT_THROW(static_cast<void>(planner.calculateRoute(network, 99, 4)), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(planner.calculateRoute(network, 1, 99)), std::invalid_argument);

    auto missingRoadMetrics = gridlockedMetrics(999);

    EXPECT_THROW(static_cast<void>(planner.calculateRoute(
                     network, 1, 4, std::span<const RoadTrafficMetrics>{&missingRoadMetrics, 1})),
                 std::invalid_argument);

    const std::array duplicateMetrics{
        gridlockedMetrics(30),
        gridlockedMetrics(30),
    };

    EXPECT_THROW(static_cast<void>(planner.calculateRoute(network, 1, 4, duplicateMetrics)),
                 std::invalid_argument);
}

} // namespace