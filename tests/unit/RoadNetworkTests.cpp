#include "trafficsim/network/RoadNetwork.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace
{

using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;

constexpr RoadProperties roadProperties()
{
    return RoadProperties{
        .origin = 1,
        .destination = 2,
        .lengthMeters = 100.0,
        .speedLimitMetersPerSecond = 10.0,
        .laneCount = 1,
        .capacity = 20,
    };
}

void addTwoIntersections(RoadNetwork &network)
{
    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {100.0, 0.0}});
}

TEST(RoadNetworkTests, StartsEmpty)
{
    const RoadNetwork network;

    EXPECT_EQ(network.intersectionCount(), 0U);
    EXPECT_EQ(network.roadCount(), 0U);
    EXPECT_FALSE(network.hasIntersection(1));
    EXPECT_FALSE(network.hasRoad(1));
}

TEST(RoadNetworkTests, AddsAndRetrievesIntersection)
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {3.0, 4.0}});

    EXPECT_EQ(network.intersectionCount(), 1U);
    EXPECT_TRUE(network.hasIntersection(1));
    EXPECT_EQ(network.getIntersection(1).id(), 1U);
    EXPECT_EQ(network.getIntersection(1).position(), (trafficsim::Position{3.0, 4.0}));
}

TEST(RoadNetworkTests, RejectsDuplicateIntersection)
{
    RoadNetwork network;
    network.addIntersection(Intersection{1, {0.0, 0.0}});

    EXPECT_THROW(network.addIntersection(Intersection{1, {1.0, 1.0}}), std::invalid_argument);
    EXPECT_EQ(network.intersectionCount(), 1U);
}

TEST(RoadNetworkTests, AddsDirectedRoadAndReportsOutgoingRoad)
{
    RoadNetwork network;
    addTwoIntersections(network);

    network.addRoad(Road{10, roadProperties()});

    ASSERT_TRUE(network.hasRoad(10));
    EXPECT_EQ(network.roadCount(), 1U);
    EXPECT_EQ(network.getRoad(10).origin(), 1U);
    EXPECT_EQ(network.getRoad(10).destination(), 2U);

    const auto originRoads = network.outgoingRoads(1);
    ASSERT_EQ(originRoads.size(), 1U);
    EXPECT_EQ(originRoads.front(), 10U);

    EXPECT_TRUE(network.outgoingRoads(2).empty());
}

TEST(RoadNetworkTests, ReturnsRoadIdentifiersInDeterministicOrder)
{
    RoadNetwork network;
    addTwoIntersections(network);

    network.addRoad(Road{30, roadProperties()});
    network.addRoad(Road{10, roadProperties()});
    network.addRoad(Road{20, roadProperties()});

    const auto roadIds = network.roadIds();

    ASSERT_EQ(roadIds.size(), 3U);
    EXPECT_EQ(roadIds[0], 10U);
    EXPECT_EQ(roadIds[1], 20U);
    EXPECT_EQ(roadIds[2], 30U);
}

TEST(RoadNetworkTests, RejectsDuplicateRoad)
{
    RoadNetwork network;
    addTwoIntersections(network);
    network.addRoad(Road{10, roadProperties()});

    EXPECT_THROW(network.addRoad(Road{10, roadProperties()}), std::invalid_argument);
    EXPECT_EQ(network.roadCount(), 1U);
    EXPECT_EQ(network.outgoingRoads(1).size(), 1U);
}

TEST(RoadNetworkTests, RejectsRoadWithMissingOrigin)
{
    RoadNetwork network;
    network.addIntersection(Intersection{2, {100.0, 0.0}});

    EXPECT_THROW(network.addRoad(Road{10, roadProperties()}), std::invalid_argument);
    EXPECT_EQ(network.roadCount(), 0U);
}

TEST(RoadNetworkTests, RejectsRoadWithMissingDestination)
{
    RoadNetwork network;
    network.addIntersection(Intersection{1, {0.0, 0.0}});

    EXPECT_THROW(network.addRoad(Road{10, roadProperties()}), std::invalid_argument);
    EXPECT_EQ(network.roadCount(), 0U);
}

TEST(RoadNetworkTests, ThrowsWhenRequestedEntityDoesNotExist)
{
    const RoadNetwork network;

    EXPECT_THROW(static_cast<void>(network.getIntersection(99)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(network.getRoad(99)), std::out_of_range);
}

TEST(RoadNetworkTests, ThrowsWhenOutgoingIntersectionDoesNotExist)
{
    const RoadNetwork network;

    EXPECT_THROW(static_cast<void>(network.outgoingRoads(99)), std::out_of_range);
}

} // namespace