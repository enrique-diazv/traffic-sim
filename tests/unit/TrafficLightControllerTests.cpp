#include "trafficsim/traffic/TrafficLightController.h"

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

RoadNetwork createControlledNetwork()
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

TEST(TrafficLightControllerTests, AddsAndQueriesIncomingRoadLights)
{
    const auto network = createControlledNetwork();
    TrafficLightController controller{3};

    controller.addLight(network, 10, TrafficLight{coordinatedTimings()});
    controller.addLight(network, 20,
                        TrafficLight{
                            coordinatedTimings(),
                            TrafficLightState::Red,
                        });

    EXPECT_EQ(controller.intersectionId(), 3U);
    EXPECT_EQ(controller.lightCount(), 2U);
    EXPECT_TRUE(controller.hasLight(10));
    EXPECT_TRUE(controller.hasLight(20));
    EXPECT_EQ(controller.stateForRoad(10), TrafficLightState::Green);
    EXPECT_EQ(controller.stateForRoad(20), TrafficLightState::Red);
    EXPECT_TRUE(controller.allowsEntry(10));
    EXPECT_FALSE(controller.allowsEntry(20));
}

TEST(TrafficLightControllerTests, CoordinatesComplementaryLights)
{
    const auto network = createControlledNetwork();
    TrafficLightController controller{3};

    controller.addLight(network, 10, TrafficLight{coordinatedTimings()});
    controller.addLight(network, 20,
                        TrafficLight{
                            coordinatedTimings(),
                            TrafficLightState::Red,
                        });

    controller.update(10.0);

    EXPECT_EQ(controller.stateForRoad(10), TrafficLightState::Yellow);
    EXPECT_EQ(controller.stateForRoad(20), TrafficLightState::Red);
    EXPECT_FALSE(controller.allowsEntry(10));
    EXPECT_FALSE(controller.allowsEntry(20));

    controller.update(2.0);

    EXPECT_EQ(controller.stateForRoad(10), TrafficLightState::Red);
    EXPECT_EQ(controller.stateForRoad(20), TrafficLightState::Green);
    EXPECT_FALSE(controller.allowsEntry(10));
    EXPECT_TRUE(controller.allowsEntry(20));
}

TEST(TrafficLightControllerTests, RejectsInvalidRoadAssociations)
{
    const auto network = createControlledNetwork();

    TrafficLightController missingIntersection{99};
    EXPECT_THROW(missingIntersection.addLight(network, 10, TrafficLight{coordinatedTimings()}),
                 std::invalid_argument);

    TrafficLightController controller{3};
    EXPECT_THROW(controller.addLight(network, 999, TrafficLight{coordinatedTimings()}),
                 std::invalid_argument);

    TrafficLightController wrongDestination{1};
    EXPECT_THROW(wrongDestination.addLight(network, 10, TrafficLight{coordinatedTimings()}),
                 std::invalid_argument);
}

TEST(TrafficLightControllerTests, RejectsDuplicateIncomingRoad)
{
    const auto network = createControlledNetwork();
    TrafficLightController controller{3};

    controller.addLight(network, 10, TrafficLight{coordinatedTimings()});

    EXPECT_THROW(controller.addLight(network, 10, TrafficLight{coordinatedTimings()}),
                 std::invalid_argument);
    EXPECT_EQ(controller.lightCount(), 1U);
}

TEST(TrafficLightControllerTests, RejectsUnknownRoadsAndInvalidUpdates)
{
    TrafficLightController controller{3};

    EXPECT_THROW(static_cast<void>(controller.getLight(999)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(controller.stateForRoad(999)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(controller.allowsEntry(999)), std::out_of_range);

    EXPECT_THROW(controller.update(-0.1), std::invalid_argument);
    EXPECT_THROW(controller.update(std::numeric_limits<double>::infinity()), std::invalid_argument);
}

TEST(TrafficLightControllerTests, ResetRestoresAllInitialStates)
{
    const auto network = createControlledNetwork();
    TrafficLightController controller{3};

    controller.addLight(network, 10, TrafficLight{coordinatedTimings()});
    controller.addLight(network, 20,
                        TrafficLight{
                            coordinatedTimings(),
                            TrafficLightState::Red,
                        });

    controller.update(12.0);
    controller.reset();

    EXPECT_EQ(controller.stateForRoad(10), TrafficLightState::Green);
    EXPECT_EQ(controller.stateForRoad(20), TrafficLightState::Red);
}

} // namespace
