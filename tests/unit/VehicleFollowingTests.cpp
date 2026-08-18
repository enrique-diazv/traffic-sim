#include "trafficsim/vehicles/Vehicle.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::Route;
using trafficsim::Vehicle;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleFollowingConstraint;
using trafficsim::VehicleState;

constexpr VehicleDynamics testDynamics()
{
    return VehicleDynamics{
        .maximumSpeedMetersPerSecond = 10.0,
        .accelerationMetersPerSecondSquared = 10.0,
        .decelerationMetersPerSecondSquared = 10.0,
    };
}

RoadNetwork createNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {100.0, 0.0}});

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 100.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 20,
        },
    });

    return network;
}

TEST(VehicleFollowingTests, StopsAtFollowingLimitWithoutOverlap)
{
    const auto network = createNetwork();

    Vehicle vehicle{
        100, 1, 2, Route{{10}, 100.0}, testDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));

    const VehicleFollowingConstraint constraint{
        .maximumPositionMeters = 3.0,
        .desiredSpeedLimitMetersPerSecond = 10.0,
    };

    vehicle.update(1.0, network, nullptr, &constraint);

    EXPECT_EQ(vehicle.state(), VehicleState::Waiting);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 3.0);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
}

TEST(VehicleFollowingTests, LimitsDesiredSpeedNearLeader)
{
    const auto network = createNetwork();

    Vehicle vehicle{
        100, 1, 2, Route{{10}, 100.0}, testDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));

    const VehicleFollowingConstraint constraint{
        .maximumPositionMeters = 100.0,
        .desiredSpeedLimitMetersPerSecond = 2.0,
    };

    vehicle.update(1.0, network, nullptr, &constraint);

    EXPECT_EQ(vehicle.state(), VehicleState::Driving);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 2.0);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 2.0);
}

TEST(VehicleFollowingTests, ResumesWhenQueueMovesForward)
{
    const auto network = createNetwork();

    Vehicle vehicle{
        100, 1, 2, Route{{10}, 100.0}, testDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));

    const VehicleFollowingConstraint blocked{
        .maximumPositionMeters = 3.0,
        .desiredSpeedLimitMetersPerSecond = 10.0,
    };

    vehicle.update(1.0, network, nullptr, &blocked);
    ASSERT_EQ(vehicle.state(), VehicleState::Waiting);

    const VehicleFollowingConstraint advanced{
        .maximumPositionMeters = 8.0,
        .desiredSpeedLimitMetersPerSecond = 10.0,
    };

    vehicle.update(0.5, network, nullptr, &advanced);

    EXPECT_EQ(vehicle.state(), VehicleState::Driving);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 5.0);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 5.5);
}

TEST(VehicleFollowingTests, RejectsInvalidConstraintsWithoutMoving)
{
    const auto network = createNetwork();

    Vehicle vehicle{
        100, 1, 2, Route{{10}, 100.0}, testDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));

    auto constraint = VehicleFollowingConstraint{
        .maximumPositionMeters = -0.1,
        .desiredSpeedLimitMetersPerSecond = 10.0,
    };

    EXPECT_THROW(vehicle.update(1.0, network, nullptr, &constraint), std::invalid_argument);

    constraint.maximumPositionMeters = std::numeric_limits<double>::infinity();

    EXPECT_THROW(vehicle.update(1.0, network, nullptr, &constraint), std::invalid_argument);

    constraint.maximumPositionMeters = 100.0;
    constraint.desiredSpeedLimitMetersPerSecond = -0.1;

    EXPECT_THROW(vehicle.update(1.0, network, nullptr, &constraint), std::invalid_argument);

    EXPECT_EQ(vehicle.state(), VehicleState::Driving);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 0.0);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
}

} // namespace
