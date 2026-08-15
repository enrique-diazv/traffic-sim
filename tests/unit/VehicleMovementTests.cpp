#include "trafficsim/vehicles/Vehicle.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::Intersection;
using trafficsim::IntersectionId;
using trafficsim::Road;
using trafficsim::RoadId;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::Route;
using trafficsim::Vehicle;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleState;

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

RoadNetwork createSingleRoadNetwork(double lengthMeters, double speedLimitMetersPerSecond)
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {lengthMeters, 0.0}});
    addRoad(network, 10, 1, 2, lengthMeters, speedLimitMetersPerSecond);

    return network;
}

RoadNetwork createTwoRoadNetwork(double firstLengthMeters, double firstSpeedLimit,
                                 double secondLengthMeters, double secondSpeedLimit)
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {firstLengthMeters, 0.0}});
    network.addIntersection(Intersection{3, {firstLengthMeters + secondLengthMeters, 0.0}});

    addRoad(network, 10, 1, 2, firstLengthMeters, firstSpeedLimit);
    addRoad(network, 20, 2, 3, secondLengthMeters, secondSpeedLimit);

    return network;
}

constexpr VehicleDynamics vehicleDynamics(double maximumSpeed, double acceleration,
                                          double deceleration)
{
    return VehicleDynamics{
        .maximumSpeedMetersPerSecond = maximumSpeed,
        .accelerationMetersPerSecondSquared = acceleration,
        .decelerationMetersPerSecondSquared = deceleration,
    };
}

TEST(VehicleMovementTests, DoesNotMoveBeforeStarting)
{
    const auto network = createSingleRoadNetwork(100.0, 10.0);
    Vehicle vehicle{
        100, 1, 2, Route{{10}, 100.0}, vehicleDynamics(10.0, 5.0, 5.0),
    };

    vehicle.update(1.0, network);

    EXPECT_EQ(vehicle.state(), VehicleState::Spawning);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 0.0);
}

TEST(VehicleMovementTests, AcceleratesWithoutExceedingLimits)
{
    const auto network = createSingleRoadNetwork(1000.0, 10.0);
    Vehicle vehicle{
        100, 1, 2, Route{{10}, 1000.0}, vehicleDynamics(20.0, 4.0, 5.0),
    };
    ASSERT_TRUE(vehicle.start(network));

    vehicle.update(1.0, network);

    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 4.0);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 4.0);

    vehicle.update(2.0, network);

    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 10.0);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 24.0);
}

TEST(VehicleMovementTests, PreservesExtraDistanceAndArrives)
{
    const auto network = createTwoRoadNetwork(10.0, 10.0, 10.0, 10.0);
    Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 20.0}, vehicleDynamics(10.0, 10.0, 10.0),
    };
    ASSERT_TRUE(vehicle.start(network));

    vehicle.update(1.5, network);

    ASSERT_TRUE(vehicle.currentRoad().has_value());
    EXPECT_EQ(*vehicle.currentRoad(), 20U);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 5.0);
    EXPECT_EQ(vehicle.state(), VehicleState::Driving);

    vehicle.update(0.5, network);

    EXPECT_EQ(vehicle.state(), VehicleState::Arrived);
    EXPECT_FALSE(vehicle.currentRoad().has_value());
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 0.0);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
}

TEST(VehicleMovementTests, DeceleratesOnRoadWithLowerSpeedLimit)
{
    const auto network = createTwoRoadNetwork(10.0, 20.0, 100.0, 5.0);
    Vehicle vehicle{
        100, 1, 3, Route{{10, 20}, 110.0}, vehicleDynamics(20.0, 20.0, 3.0),
    };
    ASSERT_TRUE(vehicle.start(network));

    vehicle.update(1.0, network);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 20.0);

    vehicle.update(1.0, network);

    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 17.0);
    EXPECT_EQ(vehicle.state(), VehicleState::Driving);
}

TEST(VehicleMovementTests, RejectsInvalidUpdateDuration)
{
    const auto network = createSingleRoadNetwork(100.0, 10.0);
    Vehicle vehicle{
        100, 1, 2, Route{{10}, 100.0}, vehicleDynamics(10.0, 5.0, 5.0),
    };
    ASSERT_TRUE(vehicle.start(network));

    EXPECT_THROW(vehicle.update(-0.1, network), std::invalid_argument);
    EXPECT_THROW(vehicle.update(std::numeric_limits<double>::infinity(), network),
                 std::invalid_argument);
    EXPECT_THROW(vehicle.update(std::numeric_limits<double>::quiet_NaN(), network),
                 std::invalid_argument);
}

} // namespace