#include "trafficsim/routing/DijkstraRoutePlanner.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace
{

using trafficsim::DijkstraRoutePlanner;
using trafficsim::Intersection;
using trafficsim::IntersectionId;
using trafficsim::Road;
using trafficsim::RoadId;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;

void addRoad(RoadNetwork &network, RoadId roadId, IntersectionId origin, IntersectionId destination,
             double lengthMeters)
{
    network.addRoad(Road{
        roadId,
        RoadProperties{
            .origin = origin,
            .destination = destination,
            .lengthMeters = lengthMeters,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 20,
        },
    });
}

RoadNetwork createRoutingNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {100.0, 0.0}});
    network.addIntersection(Intersection{3, {200.0, 0.0}});
    network.addIntersection(Intersection{4, {100.0, 50.0}});
    network.addIntersection(Intersection{5, {200.0, 50.0}});

    addRoad(network, 10, 1, 2, 100.0);
    addRoad(network, 20, 2, 3, 100.0);
    addRoad(network, 30, 2, 4, 50.0);
    addRoad(network, 40, 4, 5, 50.0);
    addRoad(network, 50, 3, 5, 400.0);

    return network;
}

TEST(DijkstraRoutePlannerTests, FindsShortestRoute)
{
    const auto network = createRoutingNetwork();
    const DijkstraRoutePlanner planner;

    const auto route = planner.calculateRoute(network, 1, 5);

    ASSERT_TRUE(route.has_value());
    ASSERT_EQ(route->segmentCount(), 3U);
    EXPECT_EQ(route->segments()[0], 10U);
    EXPECT_EQ(route->segments()[1], 30U);
    EXPECT_EQ(route->segments()[2], 40U);
    EXPECT_DOUBLE_EQ(route->totalDistanceMeters(), 200.0);
}

TEST(DijkstraRoutePlannerTests, SelectsShorterDirectRoad)
{
    auto network = createRoutingNetwork();
    addRoad(network, 60, 1, 5, 150.0);
    const DijkstraRoutePlanner planner;

    const auto route = planner.calculateRoute(network, 1, 5);

    ASSERT_TRUE(route.has_value());
    ASSERT_EQ(route->segmentCount(), 1U);
    EXPECT_EQ(route->segments().front(), 60U);
    EXPECT_DOUBLE_EQ(route->totalDistanceMeters(), 150.0);
}

TEST(DijkstraRoutePlannerTests, ReturnsNoRouteWhenDestinationIsUnreachable)
{
    const auto network = createRoutingNetwork();
    const DijkstraRoutePlanner planner;

    const auto route = planner.calculateRoute(network, 5, 1);

    EXPECT_FALSE(route.has_value());
}

TEST(DijkstraRoutePlannerTests, ReturnsEmptyRouteWhenStartEqualsDestination)
{
    const auto network = createRoutingNetwork();
    const DijkstraRoutePlanner planner;

    const auto route = planner.calculateRoute(network, 1, 1);

    ASSERT_TRUE(route.has_value());
    EXPECT_TRUE(route->isComplete());
    EXPECT_EQ(route->segmentCount(), 0U);
    EXPECT_DOUBLE_EQ(route->totalDistanceMeters(), 0.0);
}

TEST(DijkstraRoutePlannerTests, RejectsMissingEndpoints)
{
    const auto network = createRoutingNetwork();
    const DijkstraRoutePlanner planner;

    EXPECT_THROW(static_cast<void>(planner.calculateRoute(network, 99, 5)), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(planner.calculateRoute(network, 1, 99)), std::invalid_argument);
}

} // namespace