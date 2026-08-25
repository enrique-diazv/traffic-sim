#include "trafficsim/vehicles/VehicleManager.h"

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
using trafficsim::VehicleFollowingConfig;
using trafficsim::VehicleId;
using trafficsim::VehicleManager;
using trafficsim::VehicleState;

constexpr VehicleDynamics testDynamics()
{
    return VehicleDynamics{
        .maximumSpeedMetersPerSecond = 10.0,
        .accelerationMetersPerSecondSquared = 5.0,
        .decelerationMetersPerSecondSquared = 5.0,
    };
}

Vehicle makeVehicle(VehicleId vehicleId)
{
    return Vehicle{
        vehicleId, 1, 2, Route{{10}, 100.0}, testDynamics(),
    };
}

RoadNetwork createSingleRoadNetwork()
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

TEST(VehicleManagerTests, RejectsZeroCapacity)
{
    EXPECT_THROW((VehicleManager{0}), std::invalid_argument);
}

TEST(VehicleManagerTests, StoresAndValidatesFollowingConfiguration)
{
    const VehicleFollowingConfig validConfig{
        .minimumDistanceMeters = 3.0,
        .reactionTimeSeconds = 1.5,
    };

    const VehicleManager manager{2, validConfig};

    EXPECT_DOUBLE_EQ(manager.followingConfig().minimumDistanceMeters, 3.0);
    EXPECT_DOUBLE_EQ(manager.followingConfig().reactionTimeSeconds, 1.5);

    auto invalidConfig = validConfig;
    invalidConfig.minimumDistanceMeters = -0.1;

    EXPECT_THROW((VehicleManager{2, invalidConfig}), std::invalid_argument);

    invalidConfig = validConfig;
    invalidConfig.minimumDistanceMeters = std::numeric_limits<double>::infinity();

    EXPECT_THROW((VehicleManager{2, invalidConfig}), std::invalid_argument);

    invalidConfig = validConfig;
    invalidConfig.reactionTimeSeconds = 0.0;

    EXPECT_THROW((VehicleManager{2, invalidConfig}), std::invalid_argument);
}

TEST(VehicleManagerTests, AddsFindsAndPreservesVehicleOrder)
{
    VehicleManager manager{2};

    EXPECT_TRUE(manager.empty());
    EXPECT_FALSE(manager.full());
    EXPECT_EQ(manager.maximumVehicles(), 2U);

    manager.addVehicle(makeVehicle(100));

    EXPECT_FALSE(manager.empty());
    EXPECT_FALSE(manager.full());
    EXPECT_EQ(manager.vehicleCount(), 1U);
    EXPECT_TRUE(manager.hasVehicle(100));
    EXPECT_EQ(manager.getVehicle(100).id(), 100U);

    manager.addVehicle(makeVehicle(200));

    ASSERT_TRUE(manager.full());
    ASSERT_EQ(manager.vehicles().size(), 2U);
    EXPECT_EQ(manager.vehicles()[0].id(), 100U);
    EXPECT_EQ(manager.vehicles()[1].id(), 200U);

    const auto &constManager = manager;
    EXPECT_EQ(constManager.getVehicle(200).id(), 200U);
}

TEST(VehicleManagerTests, RejectsDuplicateIdentifiers)
{
    VehicleManager manager{2};
    manager.addVehicle(makeVehicle(100));

    EXPECT_THROW(manager.addVehicle(makeVehicle(100)), std::invalid_argument);
    EXPECT_EQ(manager.vehicleCount(), 1U);
}

TEST(VehicleManagerTests, RejectsVehiclesBeyondCapacity)
{
    VehicleManager manager{1};
    manager.addVehicle(makeVehicle(100));

    EXPECT_THROW(manager.addVehicle(makeVehicle(200)), std::length_error);
    EXPECT_EQ(manager.vehicleCount(), 1U);
}

TEST(VehicleManagerTests, RejectsUnknownIdentifiers)
{
    VehicleManager manager{1};

    EXPECT_THROW(static_cast<void>(manager.getVehicle(999)), std::out_of_range);
}

TEST(VehicleManagerTests, UpdatesVehiclesInRoadOrderWithoutOverlap)
{
    const auto network = createSingleRoadNetwork();
    VehicleManager manager{2};

    manager.addVehicle(makeVehicle(100));
    manager.addVehicle(makeVehicle(200));

    ASSERT_TRUE(manager.getVehicle(100).start(network));
    ASSERT_TRUE(manager.getVehicle(200).start(network));

    manager.update(1.0, network);

    const auto &leader = manager.getVehicle(100);
    const auto &follower = manager.getVehicle(200);

    EXPECT_DOUBLE_EQ(leader.speedMetersPerSecond(), 5.0);
    EXPECT_DOUBLE_EQ(leader.positionMeters(), 5.0);

    EXPECT_EQ(follower.state(), VehicleState::Waiting);
    EXPECT_DOUBLE_EQ(follower.speedMetersPerSecond(), 0.0);
    EXPECT_DOUBLE_EQ(follower.positionMeters(), 3.0);

    EXPECT_DOUBLE_EQ(leader.positionMeters() - follower.positionMeters(),
                     manager.followingConfig().minimumDistanceMeters);
}

TEST(VehicleManagerTests, BreaksPositionTiesWithoutReorderingStorage)
{
    const auto network = createSingleRoadNetwork();
    VehicleManager manager{2};

    manager.addVehicle(makeVehicle(200));
    manager.addVehicle(makeVehicle(100));

    ASSERT_TRUE(manager.getVehicle(200).start(network));
    ASSERT_TRUE(manager.getVehicle(100).start(network));

    manager.update(1.0, network);

    EXPECT_DOUBLE_EQ(manager.getVehicle(100).positionMeters(), 5.0);
    EXPECT_DOUBLE_EQ(manager.getVehicle(200).positionMeters(), 3.0);

    ASSERT_EQ(manager.vehicles().size(), 2U);
    EXPECT_EQ(manager.vehicles()[0].id(), 200U);
    EXPECT_EQ(manager.vehicles()[1].id(), 100U);
}

TEST(VehicleManagerTests, UpdatesDifferentRoadsIndependently)
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {100.0, 0.0}});
    network.addIntersection(Intersection{3, {0.0, 100.0}});
    network.addIntersection(Intersection{4, {100.0, 100.0}});

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

    network.addRoad(Road{
        20,
        RoadProperties{
            .origin = 3,
            .destination = 4,
            .lengthMeters = 100.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 20,
        },
    });

    VehicleManager manager{2};

    manager.addVehicle(Vehicle{
        100,
        1,
        2,
        Route{{10}, 100.0},
        testDynamics(),
    });
    manager.addVehicle(Vehicle{
        200,
        3,
        4,
        Route{{20}, 100.0},
        testDynamics(),
    });

    ASSERT_TRUE(manager.getVehicle(100).start(network));
    ASSERT_TRUE(manager.getVehicle(200).start(network));

    manager.update(1.0, network);

    EXPECT_DOUBLE_EQ(manager.getVehicle(100).positionMeters(), 5.0);
    EXPECT_DOUBLE_EQ(manager.getVehicle(200).positionMeters(), 5.0);
    EXPECT_EQ(manager.getVehicle(100).state(), VehicleState::Driving);
    EXPECT_EQ(manager.getVehicle(200).state(), VehicleState::Driving);
}

TEST(VehicleManagerTests, QueueAdvancesWhenLeaderMovesForward)
{
    const auto network = createSingleRoadNetwork();
    VehicleManager manager{2};

    manager.addVehicle(makeVehicle(100));
    manager.addVehicle(makeVehicle(200));

    ASSERT_TRUE(manager.getVehicle(100).start(network));
    ASSERT_TRUE(manager.getVehicle(200).start(network));

    manager.update(1.0, network);
    ASSERT_EQ(manager.getVehicle(200).state(), VehicleState::Waiting);

    manager.update(1.0, network);

    const auto &leader = manager.getVehicle(100);
    const auto &follower = manager.getVehicle(200);

    EXPECT_DOUBLE_EQ(leader.positionMeters(), 15.0);
    EXPECT_DOUBLE_EQ(leader.speedMetersPerSecond(), 10.0);

    EXPECT_EQ(follower.state(), VehicleState::Driving);
    EXPECT_DOUBLE_EQ(follower.positionMeters(), 8.0);
    EXPECT_DOUBLE_EQ(follower.speedMetersPerSecond(), 5.0);

    const auto expectedSafeDistance =
        manager.followingConfig().minimumDistanceMeters +
        (follower.speedMetersPerSecond() * manager.followingConfig().reactionTimeSeconds);

    EXPECT_DOUBLE_EQ(leader.positionMeters() - follower.positionMeters(), expectedSafeDistance);
}

TEST(VehicleManagerTests, RemovesOnlyArrivedVehicles)
{
    const auto network = createSingleRoadNetwork();
    VehicleManager manager{2};

    manager.addVehicle(makeVehicle(100));
    manager.addVehicle(makeVehicle(200));

    ASSERT_TRUE(manager.getVehicle(100).start(network));

    manager.update(20.0, network);

    EXPECT_EQ(manager.getVehicle(100).state(), VehicleState::Arrived);
    EXPECT_EQ(manager.getVehicle(200).state(), VehicleState::Spawning);

    EXPECT_EQ(manager.removeArrived(), 1U);
    EXPECT_FALSE(manager.hasVehicle(100));
    EXPECT_TRUE(manager.hasVehicle(200));
    ASSERT_EQ(manager.vehicleCount(), 1U);
    EXPECT_EQ(manager.getVehicle(200).id(), 200U);
    EXPECT_EQ(manager.vehicles()[0].id(), 200U);
}

TEST(VehicleManagerTests, ClearsVehiclesWithoutChangingCapacity)
{
    VehicleManager manager{2};
    manager.addVehicle(makeVehicle(100));
    manager.addVehicle(makeVehicle(200));

    manager.clear();

    EXPECT_TRUE(manager.empty());
    EXPECT_FALSE(manager.full());
    EXPECT_EQ(manager.vehicleCount(), 0U);
    EXPECT_EQ(manager.maximumVehicles(), 2U);
    EXPECT_FALSE(manager.hasVehicle(100));
    EXPECT_FALSE(manager.hasVehicle(200));

    manager.addVehicle(makeVehicle(100));

    EXPECT_TRUE(manager.hasVehicle(100));
    EXPECT_EQ(manager.getVehicle(100).id(), 100U);
}

} // namespace
