#include "trafficsim/experiments/ExperimentCsvExporter.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace trafficsim
{
namespace
{

constexpr std::array<std::string_view, 9> metricNames{
    "vehicles_spawned",
    "vehicles_arrived",
    "total_reroutes",
    "average_travel_time_seconds",
    "average_waiting_time_seconds",
    "average_speed_meters_per_second",
    "total_distance_meters",
    "average_route_length_meters",
    "peak_active_vehicles",
};

std::ofstream openOutputFile(const std::filesystem::path &path)
{
    std::ofstream output{path};

    if (!output.is_open())
    {
        throw std::runtime_error{"Could not open experiment CSV output file: " + path.string()};
    }

    return output;
}

void verifyWrite(const std::ostream &output)
{
    if (!output)
    {
        throw std::runtime_error{"Could not write experiment CSV output"};
    }
}

void writeCsvField(std::ostream &output, std::string_view value)
{
    if (value.find_first_of(",\"\r\n") == std::string_view::npos)
    {
        output << value;
        return;
    }

    output << '"';

    for (const auto character : value)
    {
        if (character == '"')
        {
            output << "\"\"";
        }
        else
        {
            output << character;
        }
    }

    output << '"';
}

void writeMetricHeader(std::ostream &output, std::string_view metricName)
{
    output << ',' << metricName << "_mean";
    output << ',' << metricName << "_minimum";
    output << ',' << metricName << "_maximum";
    output << ',' << metricName << "_standard_deviation";
}

void writeMetric(std::ostream &output, const AggregateMetric &metric)
{
    output << ',' << metric.mean;
    output << ',' << metric.minimum;
    output << ',' << metric.maximum;
    output << ',' << metric.standardDeviation;
}

} // namespace

void ExperimentCsvExporter::writeRunResults(std::ostream &output,
                                            std::span<const ExperimentRunResult> runResults)
{
    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();

    output << std::defaultfloat << std::setprecision(15);
    output << "variant,repetition_index,random_seed,total_reroutes,"
              "vehicles_spawned,vehicles_arrived,average_travel_time_seconds,"
              "minimum_travel_time_seconds,maximum_travel_time_seconds,"
              "average_waiting_time_seconds,average_speed_meters_per_second,"
              "total_distance_meters,average_route_length_meters,peak_active_vehicles\n";

    for (const auto &runResult : runResults)
    {
        writeCsvField(output, runResult.variantName);

        const auto &summary = runResult.summary;

        output << ',' << runResult.repetitionIndex << ',' << runResult.randomSeed << ','
               << runResult.totalReroutes << ',' << summary.vehiclesSpawned << ','
               << summary.vehiclesArrived << ',' << summary.averageTravelTimeSeconds << ','
               << summary.minimumTravelTimeSeconds << ',' << summary.maximumTravelTimeSeconds << ','
               << summary.averageWaitingTimeSeconds << ',' << summary.averageSpeedMetersPerSecond
               << ',' << summary.totalDistanceMeters << ',' << summary.averageRouteLengthMeters
               << ',' << summary.peakActiveVehicles << '\n';
    }

    output.flags(previousFlags);
    output.precision(previousPrecision);
    verifyWrite(output);
}

void ExperimentCsvExporter::writeAggregatedResults(
    std::ostream &output, std::span<const AggregatedExperimentResult> aggregatedResults)
{
    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();

    output << std::defaultfloat << std::setprecision(15);
    output << "variant,run_count";

    for (const auto metricName : metricNames)
    {
        writeMetricHeader(output, metricName);
    }

    output << '\n';

    for (const auto &result : aggregatedResults)
    {
        writeCsvField(output, result.variantName);
        output << ',' << result.runCount;

        writeMetric(output, result.vehiclesSpawned);
        writeMetric(output, result.vehiclesArrived);
        writeMetric(output, result.totalReroutes);
        writeMetric(output, result.averageTravelTimeSeconds);
        writeMetric(output, result.averageWaitingTimeSeconds);
        writeMetric(output, result.averageSpeedMetersPerSecond);
        writeMetric(output, result.totalDistanceMeters);
        writeMetric(output, result.averageRouteLengthMeters);
        writeMetric(output, result.peakActiveVehicles);

        output << '\n';
    }

    output.flags(previousFlags);
    output.precision(previousPrecision);
    verifyWrite(output);
}

void ExperimentCsvExporter::exportToDirectory(
    const std::filesystem::path &directory, std::span<const ExperimentRunResult> runResults,
    std::span<const AggregatedExperimentResult> aggregatedResults)
{
    if (directory.empty())
    {
        throw std::invalid_argument{"Experiment CSV output directory must not be empty"};
    }

    std::filesystem::create_directories(directory);

    auto runOutput = openOutputFile(directory / "experiment_runs.csv");
    auto comparisonOutput = openOutputFile(directory / "experiment_comparison.csv");

    writeRunResults(runOutput, runResults);
    writeAggregatedResults(comparisonOutput, aggregatedResults);
}

} // namespace trafficsim