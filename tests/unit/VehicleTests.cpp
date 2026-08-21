#include "trafficsim/vehicles/Vehicle.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <vector>

namespace
{

using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadId;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::Route;
using trafficsim::Vehicle;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleState;

constexpr VehicleDynamics defaultDynamics()
{
    return VehicleDynamics{
        .maximumSpeedMetersPerSecond = 20.0,
        .accelerationMetersPerSecondSquared = 2.0,
        .decelerationMetersPerSecondSquared = 4.0,
    };
}

void addRoad(RoadNetwork &network, RoadId roadId, trafficsim::IntersectionId origin,
             trafficsim::IntersectionId destination, double lengthMeters,
             double speedLimitMetersPerSecond)
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

RoadNetwork createLinearNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {100.0, 0.0}});
    network.addIntersection(Intersection{3, {200.0, 0.0}});

    addRoad(network, 10, 1, 2, 100.0, 10.0);
    addRoad(network, 20, 2, 3, 50.0, 5.0);

    return network;
}

TEST(VehicleTests, StoresInitialState)
{
    const Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 150.0}, defaultDynamics(),
    };

    EXPECT_EQ(vehicle.id(), 100U);
    EXPECT_EQ(vehicle.origin(), 1U);
    EXPECT_EQ(vehicle.destination(), 3U);
    EXPECT_EQ(vehicle.state(), VehicleState::Spawning);
    ASSERT_TRUE(vehicle.currentRoad().has_value());
    EXPECT_EQ(*vehicle.currentRoad(), 10U);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 0.0);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
    EXPECT_DOUBLE_EQ(vehicle.maximumSpeedMetersPerSecond(), 20.0);
    EXPECT_FALSE(vehicle.spawnTimeSeconds().has_value());
    EXPECT_FALSE(vehicle.arrivalTimeSeconds().has_value());
    EXPECT_FALSE(vehicle.travelTimeSeconds().has_value());
    EXPECT_DOUBLE_EQ(vehicle.waitingTimeSeconds(), 0.0);
}

TEST(VehicleTests, StartsOnlyOnce)
{
    const auto network = createLinearNetwork();
    Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 150.0}, defaultDynamics(),
    };

    EXPECT_TRUE(vehicle.start(network));
    EXPECT_EQ(vehicle.state(), VehicleState::Driving);
    EXPECT_FALSE(vehicle.start(network));
}

TEST(VehicleTests, ReplacesFutureRouteWithoutChangingCurrentMotion)
{
    auto network = createLinearNetwork();

    network.addIntersection(Intersection{4, {.x = 150.0, .y = 50.0}});
    addRoad(network, 30, 2, 4, 60.0, 10.0);
    addRoad(network, 40, 4, 3, 70.0, 10.0);

    Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 150.0}, defaultDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));
    vehicle.update(1.0, network);

    const auto positionBeforeRerouting = vehicle.positionMeters();
    const auto speedBeforeRerouting = vehicle.speedMetersPerSecond();

    ASSERT_TRUE(vehicle.reroute(network, Route{{30, 40}, 130.0}));

    EXPECT_EQ(vehicle.state(), VehicleState::Driving);
    EXPECT_EQ(vehicle.currentRoad(), 10U);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), positionBeforeRerouting);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), speedBeforeRerouting);

    ASSERT_EQ(vehicle.route().segments().size(), 3U);
    EXPECT_EQ(vehicle.route().segments()[0], 10U);
    EXPECT_EQ(vehicle.route().segments()[1], 30U);
    EXPECT_EQ(vehicle.route().segments()[2], 40U);
    EXPECT_DOUBLE_EQ(vehicle.route().totalDistanceMeters(), 230.0);
}

TEST(VehicleTests, RejectsInvalidReroutingRequests)
{
    auto network = createLinearNetwork();

    network.addIntersection(Intersection{4, {.x = 150.0, .y = 50.0}});
    addRoad(network, 30, 2, 4, 60.0, 10.0);
    addRoad(network, 40, 4, 3, 70.0, 10.0);

    Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 150.0}, defaultDynamics(),
    };

    EXPECT_FALSE(vehicle.reroute(network, Route{{30, 40}, 130.0}));

    ASSERT_TRUE(vehicle.start(network));

    EXPECT_THROW(static_cast<void>(vehicle.reroute(network, Route{{40}, 70.0})),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(vehicle.reroute(network, Route{{30}, 60.0})),
                 std::invalid_argument);

    ASSERT_EQ(vehicle.route().segments().size(), 2U);
    EXPECT_EQ(vehicle.route().segments()[0], 10U);
    EXPECT_EQ(vehicle.route().segments()[1], 20U);
}

TEST(VehicleTests, EmptyRouteArrivesImmediately)
{
    RoadNetwork network;
    network.addIntersection(Intersection{1, {0.0, 0.0}});

    Vehicle vehicle{
        100, 1, 1, Route{std::vector<RoadId>{}, 0.0}, defaultDynamics(),
    };

    EXPECT_TRUE(vehicle.start(network, 3.5));
    EXPECT_EQ(vehicle.state(), VehicleState::Arrived);
    EXPECT_FALSE(vehicle.currentRoad().has_value());

    ASSERT_TRUE(vehicle.spawnTimeSeconds().has_value());
    ASSERT_TRUE(vehicle.arrivalTimeSeconds().has_value());
    ASSERT_TRUE(vehicle.travelTimeSeconds().has_value());

    EXPECT_DOUBLE_EQ(*vehicle.spawnTimeSeconds(), 3.5);
    EXPECT_DOUBLE_EQ(*vehicle.arrivalTimeSeconds(), 3.5);
    EXPECT_DOUBLE_EQ(*vehicle.travelTimeSeconds(), 0.0);
}

TEST(VehicleTests, RecordsTripTimingWhenArriving)
{
    const auto network = createLinearNetwork();
    Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 150.0}, defaultDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network, 12.5));

    ASSERT_TRUE(vehicle.spawnTimeSeconds().has_value());
    EXPECT_DOUBLE_EQ(*vehicle.spawnTimeSeconds(), 12.5);
    EXPECT_FALSE(vehicle.arrivalTimeSeconds().has_value());
    EXPECT_FALSE(vehicle.travelTimeSeconds().has_value());

    vehicle.update(10.0, network);

    EXPECT_EQ(vehicle.state(), VehicleState::Driving);
    EXPECT_FALSE(vehicle.arrivalTimeSeconds().has_value());
    EXPECT_FALSE(vehicle.travelTimeSeconds().has_value());

    vehicle.update(10.0, network);

    EXPECT_EQ(vehicle.state(), VehicleState::Arrived);
    ASSERT_TRUE(vehicle.arrivalTimeSeconds().has_value());
    ASSERT_TRUE(vehicle.travelTimeSeconds().has_value());
    EXPECT_DOUBLE_EQ(*vehicle.arrivalTimeSeconds(), 32.5);
    EXPECT_DOUBLE_EQ(*vehicle.travelTimeSeconds(), 20.0);
}

TEST(VehicleTests, RejectsInvalidSpawnTimeWithoutStarting)
{
    const auto network = createLinearNetwork();
    Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 150.0}, defaultDynamics(),
    };

    EXPECT_THROW(static_cast<void>(vehicle.start(network, -1.0)), std::invalid_argument);
    EXPECT_EQ(vehicle.state(), VehicleState::Spawning);
    EXPECT_FALSE(vehicle.spawnTimeSeconds().has_value());
}

TEST(VehicleTests, RejectsInvalidDynamics)
{
    auto dynamics = defaultDynamics();
    dynamics.maximumSpeedMetersPerSecond = 0.0;

    EXPECT_THROW((Vehicle{100, 1, 2, Route{{10}, 100.0}, dynamics}), std::invalid_argument);

    dynamics = defaultDynamics();
    dynamics.accelerationMetersPerSecondSquared = std::numeric_limits<double>::quiet_NaN();

    EXPECT_THROW((Vehicle{100, 1, 2, Route{{10}, 100.0}, dynamics}), std::invalid_argument);

    dynamics = defaultDynamics();
    dynamics.decelerationMetersPerSecondSquared = -1.0;

    EXPECT_THROW((Vehicle{100, 1, 2, Route{{10}, 100.0}, dynamics}), std::invalid_argument);
}

TEST(VehicleTests, RejectsRouteWithMissingRoad)
{
    const auto network = createLinearNetwork();
    Vehicle vehicle{
        100, 1, 3, Route{{99}, 100.0}, defaultDynamics(),
    };

    EXPECT_THROW(static_cast<void>(vehicle.start(network)), std::invalid_argument);
    EXPECT_EQ(vehicle.state(), VehicleState::Spawning);
}

TEST(VehicleTests, RejectsDisconnectedRoute)
{
    auto network = createLinearNetwork();
    network.addIntersection(Intersection{4, {300.0, 0.0}});
    addRoad(network, 30, 3, 4, 100.0, 10.0);

    Vehicle vehicle{
        100, 1, 4, Route{{10, 30}, 200.0}, defaultDynamics(),
    };

    EXPECT_THROW(static_cast<void>(vehicle.start(network)), std::invalid_argument);
    EXPECT_EQ(vehicle.state(), VehicleState::Spawning);
}

} // namespace