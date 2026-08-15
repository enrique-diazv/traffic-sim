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

TEST(VehicleTests, EmptyRouteArrivesImmediately)
{
    RoadNetwork network;
    network.addIntersection(Intersection{1, {0.0, 0.0}});

    Vehicle vehicle{
        100, 1, 1, Route{std::vector<RoadId>{}, 0.0}, defaultDynamics(),
    };

    EXPECT_TRUE(vehicle.start(network));
    EXPECT_EQ(vehicle.state(), VehicleState::Arrived);
    EXPECT_FALSE(vehicle.currentRoad().has_value());
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