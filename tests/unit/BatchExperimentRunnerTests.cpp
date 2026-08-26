#include "trafficsim/experiments/BatchExperimentRunner.h"

#include "trafficsim/network/Intersection.h"
#include "trafficsim/network/Road.h"
#include "trafficsim/network/RoadNetwork.h"

#include <gtest/gtest.h>

#include <utility>

namespace
{

using trafficsim::BatchExperimentConfig;
using trafficsim::BatchExperimentRunner;
using trafficsim::ExperimentVariant;
using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::Scenario;
using trafficsim::VehicleSpawnRequest;

Scenario createScenario()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {.x = 0.0, .y = 0.0}});
    network.addIntersection(Intersection{2, {.x = 1.0, .y = 0.0}});

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 1.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 10,
        },
    });

    trafficsim::SimulationConfig simulationConfig;
    simulationConfig.durationSeconds = 2.0;
    simulationConfig.timeStepSeconds = 0.1;
    simulationConfig.randomSeed = 10U;
    simulationConfig.maximumVehicles = 10U;

    return Scenario{
        .config = simulationConfig,
        .roadNetwork = std::move(network),
        .trafficManager = {},
        .spawnSchedule =
            {
                VehicleSpawnRequest{
                    .spawnTimeSeconds = 0.0,
                    .origin = 1,
                    .destination = 2,
                },
            },
    };
}

BatchExperimentConfig createExperimentConfig(const Scenario &scenario)
{
    auto alternativeConfig = scenario.config;
    alternativeConfig.minimumFollowingDistanceMeters = 3.0;

    return {
        .repetitions = 2U,
        .seedStride = 5U,
        .variants =
            {
                ExperimentVariant{
                    .name = "baseline",
                    .simulationConfig = scenario.config,
                },
                ExperimentVariant{
                    .name = "alternative",
                    .simulationConfig = alternativeConfig,
                },
            },
    };
}

TEST(BatchExperimentRunnerTests, RunsEveryVariantAndRepetitionInDeterministicOrder)
{
    const auto scenario = createScenario();
    const auto config = createExperimentConfig(scenario);

    const auto results = BatchExperimentRunner::run(scenario, config);

    ASSERT_EQ(results.size(), 4U);

    EXPECT_EQ(results[0].variantName, "baseline");
    EXPECT_EQ(results[0].repetitionIndex, 0U);
    EXPECT_EQ(results[0].randomSeed, 10U);

    EXPECT_EQ(results[1].variantName, "baseline");
    EXPECT_EQ(results[1].repetitionIndex, 1U);
    EXPECT_EQ(results[1].randomSeed, 15U);

    EXPECT_EQ(results[2].variantName, "alternative");
    EXPECT_EQ(results[2].repetitionIndex, 0U);
    EXPECT_EQ(results[2].randomSeed, 10U);

    EXPECT_EQ(results[3].variantName, "alternative");
    EXPECT_EQ(results[3].repetitionIndex, 1U);
    EXPECT_EQ(results[3].randomSeed, 15U);

    for (const auto &result : results)
    {
        EXPECT_EQ(result.totalReroutes, 0U);
        EXPECT_EQ(result.summary.vehiclesSpawned, 1U);
        EXPECT_EQ(result.summary.vehiclesArrived, 1U);
    }
}

TEST(BatchExperimentRunnerTests, ParallelRunMatchesSequentialResultsAndOrder)
{
    const auto scenario = createScenario();
    const auto config = createExperimentConfig(scenario);

    const auto sequentialResults = BatchExperimentRunner::run(scenario, config);
    const auto parallelResults = BatchExperimentRunner::runParallel(scenario, config, 3U);

    ASSERT_EQ(parallelResults.size(), sequentialResults.size());

    for (std::size_t index = 0U; index < sequentialResults.size(); ++index)
    {
        const auto &sequential = sequentialResults[index];
        const auto &parallel = parallelResults[index];

        EXPECT_EQ(parallel.variantName, sequential.variantName);
        EXPECT_EQ(parallel.repetitionIndex, sequential.repetitionIndex);
        EXPECT_EQ(parallel.randomSeed, sequential.randomSeed);
        EXPECT_EQ(parallel.totalReroutes, sequential.totalReroutes);
        EXPECT_EQ(parallel.summary.vehiclesSpawned, sequential.summary.vehiclesSpawned);
        EXPECT_EQ(parallel.summary.vehiclesArrived, sequential.summary.vehiclesArrived);
        EXPECT_DOUBLE_EQ(parallel.summary.averageTravelTimeSeconds,
                         sequential.summary.averageTravelTimeSeconds);
        EXPECT_DOUBLE_EQ(parallel.summary.minimumTravelTimeSeconds,
                         sequential.summary.minimumTravelTimeSeconds);
        EXPECT_DOUBLE_EQ(parallel.summary.maximumTravelTimeSeconds,
                         sequential.summary.maximumTravelTimeSeconds);
        EXPECT_DOUBLE_EQ(parallel.summary.averageWaitingTimeSeconds,
                         sequential.summary.averageWaitingTimeSeconds);
        EXPECT_DOUBLE_EQ(parallel.summary.averageSpeedMetersPerSecond,
                         sequential.summary.averageSpeedMetersPerSecond);
        EXPECT_DOUBLE_EQ(parallel.summary.totalDistanceMeters,
                         sequential.summary.totalDistanceMeters);
        EXPECT_DOUBLE_EQ(parallel.summary.averageRouteLengthMeters,
                         sequential.summary.averageRouteLengthMeters);
        EXPECT_EQ(parallel.summary.peakActiveVehicles, sequential.summary.peakActiveVehicles);
    }
}

TEST(BatchExperimentRunnerTests, RejectsZeroParallelWorkers)
{
    const auto scenario = createScenario();
    const auto config = createExperimentConfig(scenario);

    EXPECT_THROW(static_cast<void>(BatchExperimentRunner::runParallel(scenario, config, 0U)),
                 std::invalid_argument);
}

TEST(BatchExperimentRunnerTests, ReproducesResultsForIdenticalInput)
{
    const auto scenario = createScenario();
    const auto config = createExperimentConfig(scenario);

    const auto firstResults = BatchExperimentRunner::run(scenario, config);
    const auto secondResults = BatchExperimentRunner::run(scenario, config);

    ASSERT_EQ(firstResults.size(), secondResults.size());

    for (std::size_t index = 0U; index < firstResults.size(); ++index)
    {
        const auto &first = firstResults[index];
        const auto &second = secondResults[index];

        EXPECT_EQ(first.variantName, second.variantName);
        EXPECT_EQ(first.repetitionIndex, second.repetitionIndex);
        EXPECT_EQ(first.randomSeed, second.randomSeed);
        EXPECT_EQ(first.totalReroutes, second.totalReroutes);
        EXPECT_EQ(first.summary.vehiclesSpawned, second.summary.vehiclesSpawned);
        EXPECT_EQ(first.summary.vehiclesArrived, second.summary.vehiclesArrived);
        EXPECT_DOUBLE_EQ(first.summary.averageTravelTimeSeconds,
                         second.summary.averageTravelTimeSeconds);
        EXPECT_DOUBLE_EQ(first.summary.minimumTravelTimeSeconds,
                         second.summary.minimumTravelTimeSeconds);
        EXPECT_DOUBLE_EQ(first.summary.maximumTravelTimeSeconds,
                         second.summary.maximumTravelTimeSeconds);
        EXPECT_DOUBLE_EQ(first.summary.averageWaitingTimeSeconds,
                         second.summary.averageWaitingTimeSeconds);
        EXPECT_DOUBLE_EQ(first.summary.averageSpeedMetersPerSecond,
                         second.summary.averageSpeedMetersPerSecond);
        EXPECT_DOUBLE_EQ(first.summary.totalDistanceMeters, second.summary.totalDistanceMeters);
        EXPECT_DOUBLE_EQ(first.summary.averageRouteLengthMeters,
                         second.summary.averageRouteLengthMeters);
        EXPECT_EQ(first.summary.peakActiveVehicles, second.summary.peakActiveVehicles);
    }
}

} // namespace