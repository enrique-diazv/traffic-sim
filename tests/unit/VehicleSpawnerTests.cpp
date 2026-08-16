#include "trafficsim/routing/DijkstraRoutePlanner.h"
#include "trafficsim/vehicles/VehicleSpawner.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <vector>

namespace
{

using trafficsim::DijkstraRoutePlanner;
using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleManager;
using trafficsim::VehicleSpawner;
using trafficsim::VehicleSpawnRequest;
using trafficsim::VehicleState;

constexpr VehicleDynamics testDynamics()
{
    return VehicleDynamics{
        .maximumSpeedMetersPerSecond = 10.0,
        .accelerationMetersPerSecondSquared = 5.0,
        .decelerationMetersPerSecondSquared = 5.0,
    };
}

RoadNetwork createSpawnNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {100.0, 0.0}});
    network.addIntersection(Intersection{3, {0.0, 50.0}});

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
            .origin = 1,
            .destination = 3,
            .lengthMeters = 50.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 20,
        },
    });

    return network;
}

TEST(VehicleSpawnerTests, SortsAndReportsSchedule)
{
    VehicleSpawner spawner{
        {
            {2.0, 1, 3},
            {0.0, 1, 2},
            {1.0, 1, 3},
        },
        testDynamics(),
        100,
    };

    EXPECT_EQ(spawner.pendingCount(), 3U);
    EXPECT_FALSE(spawner.complete());

    ASSERT_TRUE(spawner.nextSpawnTime().has_value());
    EXPECT_DOUBLE_EQ(*spawner.nextSpawnTime(), 0.0);
}

TEST(VehicleSpawnerTests, SpawnsDueVehiclesWithSequentialIdentifiers)
{
    const auto network = createSpawnNetwork();
    const DijkstraRoutePlanner routePlanner;
    VehicleManager manager{2};
    VehicleSpawner spawner{
        {
            {1.0, 1, 3},
            {0.0, 1, 2},
        },
        testDynamics(),
        100,
    };

    EXPECT_EQ(spawner.spawnDue(0.0, network, routePlanner, manager), 1U);

    ASSERT_TRUE(manager.hasVehicle(100));
    EXPECT_EQ(manager.getVehicle(100).destination(), 2U);
    EXPECT_EQ(manager.getVehicle(100).state(), VehicleState::Driving);
    EXPECT_EQ(spawner.pendingCount(), 1U);

    EXPECT_EQ(spawner.spawnDue(1.0, network, routePlanner, manager), 1U);

    EXPECT_TRUE(manager.hasVehicle(101));
    EXPECT_EQ(manager.getVehicle(101).destination(), 3U);
    EXPECT_TRUE(spawner.complete());
    EXPECT_FALSE(spawner.nextSpawnTime().has_value());
}

TEST(VehicleSpawnerTests, KeepsDueRequestsWhenManagerIsFull)
{
    const auto network = createSpawnNetwork();
    const DijkstraRoutePlanner routePlanner;
    VehicleManager manager{1};
    VehicleSpawner spawner{
        {
            {0.0, 1, 2},
            {0.0, 1, 3},
        },
        testDynamics(),
        100,
    };

    EXPECT_EQ(spawner.spawnDue(0.0, network, routePlanner, manager), 1U);
    EXPECT_EQ(manager.getVehicle(100).destination(), 2U);
    EXPECT_EQ(spawner.pendingCount(), 1U);

    manager.clear();

    EXPECT_EQ(spawner.spawnDue(0.0, network, routePlanner, manager), 1U);
    EXPECT_EQ(manager.getVehicle(101).destination(), 3U);
    EXPECT_TRUE(spawner.complete());
}

TEST(VehicleSpawnerTests, ResetRestoresScheduleAndIdentifiers)
{
    const auto network = createSpawnNetwork();
    const DijkstraRoutePlanner routePlanner;
    VehicleManager manager{1};
    VehicleSpawner spawner{
        {
            {0.0, 1, 2},
        },
        testDynamics(),
        100,
    };

    EXPECT_EQ(spawner.spawnDue(0.0, network, routePlanner, manager), 1U);

    manager.clear();
    spawner.reset();

    EXPECT_EQ(spawner.pendingCount(), 1U);
    EXPECT_EQ(spawner.spawnDue(0.0, network, routePlanner, manager), 1U);
    EXPECT_TRUE(manager.hasVehicle(100));
}

TEST(VehicleSpawnerTests, RejectsUnreachableRequests)
{
    const auto network = createSpawnNetwork();
    const DijkstraRoutePlanner routePlanner;
    VehicleManager manager{1};
    VehicleSpawner spawner{
        {
            {0.0, 2, 1},
        },
        testDynamics(),
    };

    EXPECT_THROW(static_cast<void>(spawner.spawnDue(0.0, network, routePlanner, manager)),
                 std::runtime_error);

    EXPECT_EQ(spawner.pendingCount(), 1U);
    EXPECT_TRUE(manager.empty());
}

TEST(VehicleSpawnerTests, RejectsInvalidTimesAndDynamics)
{
    auto dynamics = testDynamics();

    EXPECT_THROW((VehicleSpawner{
                     std::vector<VehicleSpawnRequest>{{-1.0, 1, 2}},
                     dynamics,
                 }),
                 std::invalid_argument);

    EXPECT_THROW((VehicleSpawner{
                     std::vector<VehicleSpawnRequest>{{
                         std::numeric_limits<double>::infinity(),
                         1,
                         2,
                     }},
                     dynamics,
                 }),
                 std::invalid_argument);

    dynamics.maximumSpeedMetersPerSecond = 0.0;

    EXPECT_THROW((VehicleSpawner{
                     std::vector<VehicleSpawnRequest>{},
                     dynamics,
                 }),
                 std::invalid_argument);
}

} // namespace
