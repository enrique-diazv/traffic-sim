#include "trafficsim/traffic/TrafficManager.h"
#include "trafficsim/vehicles/Vehicle.h"
#include "trafficsim/vehicles/VehicleManager.h"

#include <gtest/gtest.h>

#include <utility>

namespace
{

using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::Route;
using trafficsim::TrafficLight;
using trafficsim::TrafficLightController;
using trafficsim::TrafficLightState;
using trafficsim::TrafficLightTimings;
using trafficsim::TrafficManager;
using trafficsim::Vehicle;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleManager;
using trafficsim::VehicleState;

constexpr TrafficLightTimings testTimings()
{
    return TrafficLightTimings{
        .greenSeconds = 10.0,
        .yellowSeconds = 2.0,
        .redSeconds = 2.0,
    };
}

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
    network.addIntersection(Intersection{2, {10.0, 0.0}});

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 10.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 20,
        },
    });

    return network;
}

TrafficManager createRedTrafficManager(const RoadNetwork &network)
{
    TrafficLightController controller{2};

    controller.addLight(network, 10,
                        TrafficLight{
                            testTimings(),
                            TrafficLightState::Red,
                        });

    TrafficManager manager;
    manager.addController(network, std::move(controller));

    return manager;
}

TEST(VehicleTrafficLightTests, StopsAtRedLightBeforeLeavingRoad)
{
    const auto network = createNetwork();
    const auto trafficManager = createRedTrafficManager(network);

    Vehicle vehicle{
        100, 1, 2, Route{{10}, 10.0}, testDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));

    vehicle.update(1.0, network, &trafficManager);

    EXPECT_EQ(vehicle.state(), VehicleState::StoppedAtLight);
    ASSERT_TRUE(vehicle.currentRoad().has_value());
    EXPECT_EQ(*vehicle.currentRoad(), 10U);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 10.0);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
}

TEST(VehicleTrafficLightTests, RemainsStoppedWhileLightIsRed)
{
    const auto network = createNetwork();
    const auto trafficManager = createRedTrafficManager(network);

    Vehicle vehicle{
        100, 1, 2, Route{{10}, 10.0}, testDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));

    vehicle.update(1.0, network, &trafficManager);
    ASSERT_EQ(vehicle.state(), VehicleState::StoppedAtLight);
    EXPECT_DOUBLE_EQ(vehicle.waitingTimeSeconds(), 0.0);

    vehicle.update(1.0, network, &trafficManager);

    EXPECT_EQ(vehicle.state(), VehicleState::StoppedAtLight);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 10.0);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
    EXPECT_DOUBLE_EQ(vehicle.waitingTimeSeconds(), 1.0);
}

TEST(VehicleTrafficLightTests, ResumesAndArrivesWhenLightTurnsGreen)
{
    const auto network = createNetwork();
    auto trafficManager = createRedTrafficManager(network);

    Vehicle vehicle{
        100, 1, 2, Route{{10}, 10.0}, testDynamics(),
    };

    ASSERT_TRUE(vehicle.start(network));

    vehicle.update(1.0, network, &trafficManager);
    ASSERT_EQ(vehicle.state(), VehicleState::StoppedAtLight);

    trafficManager.update(2.0);
    ASSERT_TRUE(trafficManager.allowsEntry(10));

    vehicle.update(0.1, network, &trafficManager);

    EXPECT_EQ(vehicle.state(), VehicleState::Arrived);
    EXPECT_FALSE(vehicle.currentRoad().has_value());
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 0.0);
    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 0.0);
}

TEST(VehicleTrafficLightTests, QueuesVehiclesBehindRedLight)
{
    const auto network = createNetwork();
    auto trafficManager = createRedTrafficManager(network);
    VehicleManager vehicleManager{2};

    vehicleManager.addVehicle(Vehicle{
        100,
        1,
        2,
        Route{{10}, 10.0},
        testDynamics(),
    });
    vehicleManager.addVehicle(Vehicle{
        200,
        1,
        2,
        Route{{10}, 10.0},
        testDynamics(),
    });

    ASSERT_TRUE(vehicleManager.getVehicle(100).start(network));
    ASSERT_TRUE(vehicleManager.getVehicle(200).start(network));

    vehicleManager.update(1.0, network, &trafficManager);

    const auto &leaderAtRed = vehicleManager.getVehicle(100);
    const auto &followerAtRed = vehicleManager.getVehicle(200);

    EXPECT_EQ(leaderAtRed.state(), VehicleState::StoppedAtLight);
    EXPECT_DOUBLE_EQ(leaderAtRed.positionMeters(), 10.0);

    EXPECT_EQ(followerAtRed.state(), VehicleState::Waiting);
    EXPECT_DOUBLE_EQ(followerAtRed.positionMeters(), 8.0);
    EXPECT_DOUBLE_EQ(followerAtRed.speedMetersPerSecond(), 0.0);

    trafficManager.update(2.0);
    vehicleManager.update(0.1, network, &trafficManager);

    EXPECT_EQ(vehicleManager.getVehicle(100).state(), VehicleState::Arrived);
    EXPECT_EQ(vehicleManager.getVehicle(200).state(), VehicleState::Driving);
    EXPECT_DOUBLE_EQ(vehicleManager.getVehicle(200).positionMeters(), 8.1);
}

} // namespace
