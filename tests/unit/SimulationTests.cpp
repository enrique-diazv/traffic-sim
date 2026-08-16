#include "trafficsim/core/Simulation.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

namespace
{

using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::Simulation;
using trafficsim::SimulationConfig;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleSpawnRequest;

RoadNetwork createNetwork(double roadLengthMeters)
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {roadLengthMeters, 0.0}});

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = roadLengthMeters,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 20,
        },
    });

    return network;
}

SimulationConfig createConfig(double durationSeconds, double timeStepSeconds)
{
    SimulationConfig config;

    config.durationSeconds = durationSeconds;
    config.timeStepSeconds = timeStepSeconds;
    config.maximumVehicles = 10;
    config.defaultVehicleDynamics = VehicleDynamics{
        .maximumSpeedMetersPerSecond = 10.0,
        .accelerationMetersPerSecondSquared = 10.0,
        .decelerationMetersPerSecondSquared = 10.0,
    };

    return config;
}

TEST(SimulationTests, StartsWithEmptyDeterministicState)
{
    Simulation simulation{
        createConfig(1.0, 0.1),
        createNetwork(100.0),
        {},
    };

    EXPECT_FALSE(simulation.finished());
    EXPECT_EQ(simulation.clock().tickCount(), 0U);
    EXPECT_DOUBLE_EQ(simulation.clock().currentTimeSeconds(), 0.0);
    EXPECT_TRUE(simulation.vehicleManager().empty());
    EXPECT_EQ(simulation.totalSpawnedVehicles(), 0U);
    EXPECT_EQ(simulation.totalArrivedVehicles(), 0U);
    EXPECT_EQ(simulation.roadNetwork().roadCount(), 1U);
}

TEST(SimulationTests, StepSpawnsUpdatesAndAdvancesClock)
{
    Simulation simulation{
        createConfig(1.0, 0.1),
        createNetwork(100.0),
        {
            {0.0, 1, 2},
        },
    };

    simulation.step();

    EXPECT_EQ(simulation.clock().tickCount(), 1U);
    EXPECT_DOUBLE_EQ(simulation.clock().currentTimeSeconds(), 0.1);
    EXPECT_EQ(simulation.totalSpawnedVehicles(), 1U);
    EXPECT_EQ(simulation.totalArrivedVehicles(), 0U);

    const auto &vehicle = simulation.vehicleManager().getVehicle(1);

    EXPECT_DOUBLE_EQ(vehicle.speedMetersPerSecond(), 1.0);
    EXPECT_DOUBLE_EQ(vehicle.positionMeters(), 0.1);
}

TEST(SimulationTests, RunUsesFixedStepsAndCountsArrivals)
{
    Simulation simulation{
        createConfig(1.0, 0.1),
        createNetwork(1.0),
        {
            {0.0, 1, 2},
        },
    };

    simulation.run();

    EXPECT_TRUE(simulation.finished());
    EXPECT_EQ(simulation.clock().tickCount(), 10U);
    EXPECT_DOUBLE_EQ(simulation.clock().currentTimeSeconds(), 1.0);
    EXPECT_EQ(simulation.totalSpawnedVehicles(), 1U);
    EXPECT_EQ(simulation.totalArrivedVehicles(), 1U);
    EXPECT_TRUE(simulation.vehicleManager().empty());
}

TEST(SimulationTests, ResetRestoresInitialStateAndSchedule)
{
    Simulation simulation{
        createConfig(1.0, 0.1),
        createNetwork(100.0),
        {
            {0.0, 1, 2},
        },
    };

    simulation.step();
    simulation.reset();

    EXPECT_FALSE(simulation.finished());
    EXPECT_EQ(simulation.clock().tickCount(), 0U);
    EXPECT_TRUE(simulation.vehicleManager().empty());
    EXPECT_EQ(simulation.totalSpawnedVehicles(), 0U);
    EXPECT_EQ(simulation.totalArrivedVehicles(), 0U);

    simulation.step();

    EXPECT_TRUE(simulation.vehicleManager().hasVehicle(1));
    EXPECT_EQ(simulation.totalSpawnedVehicles(), 1U);
}

TEST(SimulationTests, RejectsStepsAfterConfiguredDuration)
{
    Simulation simulation{
        createConfig(0.2, 0.1),
        createNetwork(100.0),
        {},
    };

    simulation.run();

    EXPECT_THROW(simulation.step(), std::logic_error);
}

TEST(SimulationTests, SameInputsProduceSameState)
{
    const auto config = createConfig(1.0, 0.1);
    const std::vector<VehicleSpawnRequest> schedule{
        {0.0, 1, 2},
    };

    Simulation first{config, createNetwork(100.0), schedule};
    Simulation second{config, createNetwork(100.0), schedule};

    first.step();
    first.step();
    first.step();

    second.step();
    second.step();
    second.step();

    const auto &firstVehicle = first.vehicleManager().getVehicle(1);
    const auto &secondVehicle = second.vehicleManager().getVehicle(1);

    EXPECT_EQ(first.clock().tickCount(), second.clock().tickCount());
    EXPECT_DOUBLE_EQ(firstVehicle.speedMetersPerSecond(), secondVehicle.speedMetersPerSecond());
    EXPECT_DOUBLE_EQ(firstVehicle.positionMeters(), secondVehicle.positionMeters());
}

} // namespace
