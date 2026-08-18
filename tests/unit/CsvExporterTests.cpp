#include "trafficsim/statistics/CsvExporter.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{

using trafficsim::CsvExporter;
using trafficsim::RoadResult;
using trafficsim::SimulationSummary;
using trafficsim::VehicleResult;

TEST(CsvExporterTests, WritesSimulationSummary)
{
    const SimulationSummary summary{
        .vehiclesSpawned = 5,
        .vehiclesArrived = 4,
        .averageTravelTimeSeconds = 12.5,
        .minimumTravelTimeSeconds = 8.0,
        .maximumTravelTimeSeconds = 20.0,
        .averageWaitingTimeSeconds = 2.25,
        .averageSpeedMetersPerSecond = 10.0,
        .totalDistanceMeters = 400.0,
        .averageRouteLengthMeters = 100.0,
        .peakActiveVehicles = 3,
    };

    std::ostringstream output;
    CsvExporter::writeSimulationSummary(output, summary);

    EXPECT_EQ(output.str(),
              "vehicles_spawned,vehicles_arrived,average_travel_time_seconds,"
              "minimum_travel_time_seconds,maximum_travel_time_seconds,"
              "average_waiting_time_seconds,average_speed_meters_per_second,"
              "total_distance_meters,average_route_length_meters,peak_active_vehicles\n"
              "5,4,12.5,8,20,2.25,10,400,100,3\n");
}

TEST(CsvExporterTests, WritesVehicleResults)
{
    const std::vector<VehicleResult> results{
        VehicleResult{
            .vehicleId = 100,
            .origin = 1,
            .destination = 3,
            .spawnTimeSeconds = 2.0,
            .arrivalTimeSeconds = 7.0,
            .travelTimeSeconds = 5.0,
            .waitingTimeSeconds = 1.5,
            .distanceMeters = 50.0,
            .averageSpeedMetersPerSecond = 10.0,
        },
    };

    std::ostringstream output;
    CsvExporter::writeVehicleResults(output, results);

    EXPECT_EQ(output.str(), "vehicle_id,origin,destination,spawn_time_seconds,arrival_time_seconds,"
                            "travel_time_seconds,waiting_time_seconds,distance_meters,"
                            "average_speed_meters_per_second\n"
                            "100,1,3,2,7,5,1.5,50,10\n");
}

TEST(CsvExporterTests, WritesRoadResults)
{
    const std::vector<RoadResult> results{
        RoadResult{
            .roadId = 10,
            .averageSpeedMetersPerSecond = 7.5,
            .peakVehicleCount = 4,
            .averageOccupancy = 0.25,
            .congestionTimeSeconds = 3.0,
        },
    };

    std::ostringstream output;
    CsvExporter::writeRoadResults(output, results);

    EXPECT_EQ(output.str(), "road_id,average_speed_meters_per_second,peak_vehicle_count,"
                            "average_occupancy,congestion_time_seconds\n"
                            "10,7.5,4,0.25,3\n");
}

TEST(CsvExporterTests, RejectsEmptyOutputDirectory)
{
    const SimulationSummary summary;
    const std::vector<VehicleResult> vehicleResults;
    const std::vector<RoadResult> roadResults;

    EXPECT_THROW(CsvExporter::exportToDirectory(std::filesystem::path{}, summary, vehicleResults,
                                                roadResults),
                 std::invalid_argument);
}

} // namespace