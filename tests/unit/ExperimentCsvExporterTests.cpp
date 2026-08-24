#include "trafficsim/experiments/ExperimentCsvExporter.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using trafficsim::AggregatedExperimentResult;
using trafficsim::AggregateMetric;
using trafficsim::ExperimentCsvExporter;
using trafficsim::ExperimentRunResult;
using trafficsim::SimulationSummary;

TEST(ExperimentCsvExporterTests, WritesPerRunResultsAndEscapesVariant)
{
    const std::vector<ExperimentRunResult> runResults{
        ExperimentRunResult{
            .variantName = "baseline,\"quoted\"",
            .repetitionIndex = 2U,
            .randomSeed = 42U,
            .totalReroutes = 3U,
            .summary =
                SimulationSummary{
                    .vehiclesSpawned = 5U,
                    .vehiclesArrived = 4U,
                    .averageTravelTimeSeconds = 12.5,
                    .minimumTravelTimeSeconds = 8.0,
                    .maximumTravelTimeSeconds = 20.0,
                    .averageWaitingTimeSeconds = 2.25,
                    .averageSpeedMetersPerSecond = 10.0,
                    .totalDistanceMeters = 400.0,
                    .averageRouteLengthMeters = 100.0,
                    .peakActiveVehicles = 3U,
                },
        },
    };

    std::ostringstream output;
    ExperimentCsvExporter::writeRunResults(output, runResults);

    EXPECT_EQ(output.str(),
              "variant,repetition_index,random_seed,total_reroutes,"
              "vehicles_spawned,vehicles_arrived,average_travel_time_seconds,"
              "minimum_travel_time_seconds,maximum_travel_time_seconds,"
              "average_waiting_time_seconds,average_speed_meters_per_second,"
              "total_distance_meters,average_route_length_meters,peak_active_vehicles\n"
              "\"baseline,\"\"quoted\"\"\",2,42,3,5,4,12.5,8,20,2.25,10,400,100,3\n");
}

TEST(ExperimentCsvExporterTests, WritesAggregatedComparison)
{
    const AggregateMetric metric{
        .mean = 2.0,
        .minimum = 1.0,
        .maximum = 3.0,
        .standardDeviation = 0.5,
    };

    const std::vector<AggregatedExperimentResult> aggregatedResults{
        AggregatedExperimentResult{
            .variantName = "baseline",
            .runCount = 2U,
            .vehiclesSpawned = metric,
            .vehiclesArrived = metric,
            .totalReroutes = metric,
            .averageTravelTimeSeconds = metric,
            .averageWaitingTimeSeconds = metric,
            .averageSpeedMetersPerSecond = metric,
            .totalDistanceMeters = metric,
            .averageRouteLengthMeters = metric,
            .peakActiveVehicles = metric,
        },
    };

    std::ostringstream output;
    ExperimentCsvExporter::writeAggregatedResults(output, aggregatedResults);

    const auto csv = output.str();

    EXPECT_TRUE(csv.starts_with("variant,run_count,vehicles_spawned_mean,vehicles_spawned_minimum,"
                                "vehicles_spawned_maximum,vehicles_spawned_standard_deviation"));

    EXPECT_NE(csv.find(",peak_active_vehicles_standard_deviation\n"), std::string::npos);

    std::string expectedRow{"baseline,2"};

    for (std::size_t metricIndex = 0U; metricIndex < 9U; ++metricIndex)
    {
        expectedRow += ",2,1,3,0.5";
    }

    expectedRow += '\n';

    EXPECT_TRUE(csv.ends_with(expectedRow));
}

TEST(ExperimentCsvExporterTests, RejectsEmptyOutputDirectory)
{
    const std::vector<ExperimentRunResult> runResults;
    const std::vector<AggregatedExperimentResult> aggregatedResults;

    EXPECT_THROW(ExperimentCsvExporter::exportToDirectory(std::filesystem::path{}, runResults,
                                                          aggregatedResults),
                 std::invalid_argument);
}

} // namespace