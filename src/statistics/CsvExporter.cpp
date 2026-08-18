#include "trafficsim/statistics/CsvExporter.h"

#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>

namespace trafficsim
{

namespace
{

std::ofstream openOutputFile(const std::filesystem::path &path)
{
    std::ofstream output{path};

    if (!output.is_open())
    {
        throw std::runtime_error{"Could not open CSV output file: " + path.string()};
    }

    return output;
}

void verifyWrite(const std::ostream &output)
{
    if (!output)
    {
        throw std::runtime_error{"Could not write CSV output"};
    }
}

} // namespace

void CsvExporter::writeSimulationSummary(std::ostream &output, const SimulationSummary &summary)
{
    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();

    output << std::defaultfloat << std::setprecision(15);
    output << "vehicles_spawned,vehicles_arrived,average_travel_time_seconds,"
              "minimum_travel_time_seconds,maximum_travel_time_seconds,"
              "average_waiting_time_seconds,average_speed_meters_per_second,"
              "total_distance_meters,average_route_length_meters,peak_active_vehicles\n";

    output << summary.vehiclesSpawned << ',' << summary.vehiclesArrived << ','
           << summary.averageTravelTimeSeconds << ',' << summary.minimumTravelTimeSeconds << ','
           << summary.maximumTravelTimeSeconds << ',' << summary.averageWaitingTimeSeconds << ','
           << summary.averageSpeedMetersPerSecond << ',' << summary.totalDistanceMeters << ','
           << summary.averageRouteLengthMeters << ',' << summary.peakActiveVehicles << '\n';

    output.flags(previousFlags);
    output.precision(previousPrecision);
    verifyWrite(output);
}

void CsvExporter::writeVehicleResults(std::ostream &output,
                                      std::span<const VehicleResult> vehicleResults)
{
    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();

    output << std::defaultfloat << std::setprecision(15);
    output << "vehicle_id,origin,destination,spawn_time_seconds,arrival_time_seconds,"
              "travel_time_seconds,waiting_time_seconds,distance_meters,"
              "average_speed_meters_per_second\n";

    for (const auto &vehicle : vehicleResults)
    {
        output << vehicle.vehicleId << ',' << vehicle.origin << ',' << vehicle.destination << ','
               << vehicle.spawnTimeSeconds << ',' << vehicle.arrivalTimeSeconds << ','
               << vehicle.travelTimeSeconds << ',' << vehicle.waitingTimeSeconds << ','
               << vehicle.distanceMeters << ',' << vehicle.averageSpeedMetersPerSecond << '\n';
    }

    output.flags(previousFlags);
    output.precision(previousPrecision);
    verifyWrite(output);
}

void CsvExporter::writeRoadResults(std::ostream &output, std::span<const RoadResult> roadResults)
{
    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();

    output << std::defaultfloat << std::setprecision(15);
    output << "road_id,average_speed_meters_per_second,peak_vehicle_count,"
              "average_occupancy,congestion_time_seconds\n";

    for (const auto &road : roadResults)
    {
        output << road.roadId << ',' << road.averageSpeedMetersPerSecond << ','
               << road.peakVehicleCount << ',' << road.averageOccupancy << ','
               << road.congestionTimeSeconds << '\n';
    }

    output.flags(previousFlags);
    output.precision(previousPrecision);
    verifyWrite(output);
}

void CsvExporter::exportToDirectory(const std::filesystem::path &directory,
                                    const SimulationSummary &summary,
                                    std::span<const VehicleResult> vehicleResults,
                                    std::span<const RoadResult> roadResults)
{
    if (directory.empty())
    {
        throw std::invalid_argument{"CSV output directory must not be empty"};
    }

    std::filesystem::create_directories(directory);

    auto summaryOutput = openOutputFile(directory / "simulation_summary.csv");
    auto vehicleOutput = openOutputFile(directory / "vehicle_results.csv");
    auto roadOutput = openOutputFile(directory / "road_metrics.csv");

    writeSimulationSummary(summaryOutput, summary);
    writeVehicleResults(vehicleOutput, vehicleResults);
    writeRoadResults(roadOutput, roadResults);
}

} // namespace trafficsim