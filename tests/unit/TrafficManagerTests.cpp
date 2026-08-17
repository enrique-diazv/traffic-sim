#include "trafficsim/traffic/TrafficManager.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::TrafficLight;
using trafficsim::TrafficLightController;
using trafficsim::TrafficLightState;
using trafficsim::TrafficLightTimings;
using trafficsim::TrafficManager;

constexpr TrafficLightTimings coordinatedTimings()
{
    return TrafficLightTimings{
        .greenSeconds = 10.0,
        .yellowSeconds = 2.0,
        .redSeconds = 12.0,
    };
}

void addRoad(RoadNetwork &network, trafficsim::RoadId roadId, trafficsim::IntersectionId origin,
             trafficsim::IntersectionId destination)
{
    network.addRoad(Road{
        roadId,
        RoadProperties{
            .origin = origin,
            .destination = destination,
            .lengthMeters = 100.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 20,
        },
    });
}

RoadNetwork createTrafficNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {-100.0, 0.0}});
    network.addIntersection(Intersection{2, {0.0, -100.0}});
    network.addIntersection(Intersection{3, {0.0, 0.0}});

    addRoad(network, 10, 1, 3);
    addRoad(network, 20, 2, 3);
    addRoad(network, 30, 3, 1);

    return network;
}

TrafficLightController createController(const RoadNetwork &network)
{
    TrafficLightController controller{3};

    controller.addLight(network, 10, TrafficLight{coordinatedTimings()});
    controller.addLight(network, 20,
                        TrafficLight{
                            coordinatedTimings(),
                            TrafficLightState::Red,
                        });

    return controller;
}

TEST(TrafficManagerTests, AddsAndFindsIntersectionController)
{
    const auto network = createTrafficNetwork();
    TrafficManager manager;

    manager.addController(network, createController(network));

    EXPECT_EQ(manager.controllerCount(), 1U);
    EXPECT_TRUE(manager.hasController(3));
    EXPECT_EQ(manager.getController(3).intersectionId(), 3U);
}

TEST(TrafficManagerTests, AnswersRoadSignalQueries)
{
    const auto network = createTrafficNetwork();
    TrafficManager manager;

    manager.addController(network, createController(network));

    ASSERT_TRUE(manager.stateForRoad(10).has_value());
    EXPECT_EQ(*manager.stateForRoad(10), TrafficLightState::Green);
    EXPECT_TRUE(manager.allowsEntry(10));

    ASSERT_TRUE(manager.stateForRoad(20).has_value());
    EXPECT_EQ(*manager.stateForRoad(20), TrafficLightState::Red);
    EXPECT_FALSE(manager.allowsEntry(20));

    EXPECT_FALSE(manager.stateForRoad(30).has_value());
    EXPECT_TRUE(manager.allowsEntry(30));
    EXPECT_TRUE(manager.allowsEntry(999));
}

TEST(TrafficManagerTests, RejectsMissingAndDuplicateControllers)
{
    const auto network = createTrafficNetwork();
    TrafficManager manager;

    EXPECT_THROW(manager.addController(network, TrafficLightController{99}), std::invalid_argument);

    manager.addController(network, createController(network));

    EXPECT_THROW(manager.addController(network, createController(network)), std::invalid_argument);
    EXPECT_EQ(manager.controllerCount(), 1U);
}

TEST(TrafficManagerTests, UpdatesAndResetsAllControllers)
{
    const auto network = createTrafficNetwork();
    TrafficManager manager;

    manager.addController(network, createController(network));

    manager.update(12.0);

    EXPECT_EQ(*manager.stateForRoad(10), TrafficLightState::Red);
    EXPECT_EQ(*manager.stateForRoad(20), TrafficLightState::Green);

    manager.reset();

    EXPECT_EQ(*manager.stateForRoad(10), TrafficLightState::Green);
    EXPECT_EQ(*manager.stateForRoad(20), TrafficLightState::Red);
}

TEST(TrafficManagerTests, RejectsUnknownControllersAndInvalidUpdates)
{
    TrafficManager manager;

    EXPECT_THROW(static_cast<void>(manager.getController(999)), std::out_of_range);

    EXPECT_THROW(manager.update(-0.1), std::invalid_argument);
    EXPECT_THROW(manager.update(std::numeric_limits<double>::infinity()), std::invalid_argument);
}

} // namespace
