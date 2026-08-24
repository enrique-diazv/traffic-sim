#include "trafficsim/experiments/ExperimentAggregator.h"

#include <gtest/gtest.h>

#include <limits>
#include <span>
#include <vector>

namespace
{

using trafficsim::ExperimentAggregator;
using trafficsim::ExperimentRunResult;
using trafficsim::SimulationSummary;

TEST(ExperimentAggregatorTests, AggregatesVariantsInFirstAppearanceOrder)
{
    const std::vector<ExperimentRunResult> runResults{
        ExperimentRunResult{
            .variantName = "baseline",
            .repetitionIndex = 0U,
            .randomSeed = 10U,
            .totalReroutes = 2U,
            .summary =
                SimulationSummary{
                    .vehiclesSpawned = 10U,
                    .vehiclesArrived = 8U,
                    .averageTravelTimeSeconds = 10.0,
                    .averageWaitingTimeSeconds = 2.0,
                    .averageSpeedMetersPerSecond = 5.0,
                    .totalDistanceMeters = 100.0,
                    .averageRouteLengthMeters = 10.0,
                    .peakActiveVehicles = 4U,
                },
        },
        ExperimentRunResult{
            .variantName = "baseline",
            .repetitionIndex = 1U,
            .randomSeed = 15U,
            .totalReroutes = 4U,
            .summary =
                SimulationSummary{
                    .vehiclesSpawned = 14U,
                    .vehiclesArrived = 12U,
                    .averageTravelTimeSeconds = 14.0,
                    .averageWaitingTimeSeconds = 4.0,
                    .averageSpeedMetersPerSecond = 7.0,
                    .totalDistanceMeters = 140.0,
                    .averageRouteLengthMeters = 12.0,
                    .peakActiveVehicles = 6U,
                },
        },
        ExperimentRunResult{
            .variantName = "alternative",
            .repetitionIndex = 0U,
            .randomSeed = 10U,
            .totalReroutes = 1U,
            .summary =
                SimulationSummary{
                    .vehiclesSpawned = 20U,
                    .vehiclesArrived = 20U,
                    .averageTravelTimeSeconds = 8.0,
                    .averageWaitingTimeSeconds = 1.0,
                    .averageSpeedMetersPerSecond = 8.0,
                    .totalDistanceMeters = 200.0,
                    .averageRouteLengthMeters = 10.0,
                    .peakActiveVehicles = 3U,
                },
        },
    };

    const auto aggregatedResults = ExperimentAggregator::aggregate(runResults);

    ASSERT_EQ(aggregatedResults.size(), 2U);

    const auto &baseline = aggregatedResults[0];
    EXPECT_EQ(baseline.variantName, "baseline");
    EXPECT_EQ(baseline.runCount, 2U);

    EXPECT_DOUBLE_EQ(baseline.vehiclesSpawned.mean, 12.0);
    EXPECT_DOUBLE_EQ(baseline.vehiclesSpawned.minimum, 10.0);
    EXPECT_DOUBLE_EQ(baseline.vehiclesSpawned.maximum, 14.0);
    EXPECT_DOUBLE_EQ(baseline.vehiclesSpawned.standardDeviation, 2.0);

    EXPECT_DOUBLE_EQ(baseline.totalReroutes.mean, 3.0);
    EXPECT_DOUBLE_EQ(baseline.totalReroutes.standardDeviation, 1.0);
    EXPECT_DOUBLE_EQ(baseline.averageTravelTimeSeconds.mean, 12.0);
    EXPECT_DOUBLE_EQ(baseline.averageTravelTimeSeconds.standardDeviation, 2.0);
    EXPECT_DOUBLE_EQ(baseline.averageWaitingTimeSeconds.mean, 3.0);
    EXPECT_DOUBLE_EQ(baseline.averageSpeedMetersPerSecond.mean, 6.0);
    EXPECT_DOUBLE_EQ(baseline.totalDistanceMeters.mean, 120.0);
    EXPECT_DOUBLE_EQ(baseline.averageRouteLengthMeters.mean, 11.0);
    EXPECT_DOUBLE_EQ(baseline.peakActiveVehicles.mean, 5.0);

    const auto &alternative = aggregatedResults[1];
    EXPECT_EQ(alternative.variantName, "alternative");
    EXPECT_EQ(alternative.runCount, 1U);
    EXPECT_DOUBLE_EQ(alternative.averageTravelTimeSeconds.mean, 8.0);
    EXPECT_DOUBLE_EQ(alternative.averageTravelTimeSeconds.standardDeviation, 0.0);
}

TEST(ExperimentAggregatorTests, RejectsEmptyOrUnnamedResults)
{
    EXPECT_THROW(
        static_cast<void>(ExperimentAggregator::aggregate(std::span<const ExperimentRunResult>{})),
        std::invalid_argument);

    const std::vector<ExperimentRunResult> unnamedResults{
        ExperimentRunResult{
            .variantName = "",
        },
    };

    EXPECT_THROW(static_cast<void>(ExperimentAggregator::aggregate(unnamedResults)),
                 std::invalid_argument);
}

TEST(ExperimentAggregatorTests, RejectsNonFiniteMetricValues)
{
    const std::vector<ExperimentRunResult> runResults{
        ExperimentRunResult{
            .variantName = "baseline",
            .summary =
                SimulationSummary{
                    .averageTravelTimeSeconds = std::numeric_limits<double>::quiet_NaN(),
                },
        },
    };

    EXPECT_THROW(static_cast<void>(ExperimentAggregator::aggregate(runResults)),
                 std::invalid_argument);
}

} // namespace