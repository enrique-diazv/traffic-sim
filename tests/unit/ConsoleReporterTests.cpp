#include "trafficsim/statistics/ConsoleReporter.h"

#include <gtest/gtest.h>

#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{

using trafficsim::ConsoleReporter;
using trafficsim::RoadResult;
using trafficsim::SimulationSummary;

TEST(ConsoleReporterTests, WritesReadableReportAndRestoresStreamFormatting)
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

    const std::vector<RoadResult> roadResults{
        RoadResult{
            .roadId = 10,
            .congestionTimeSeconds = 4.0,
        },
        RoadResult{
            .roadId = 20,
        },
    };

    std::ostringstream output;
    output << std::scientific << std::setprecision(4);

    const auto originalFlags = output.flags();
    const auto originalPrecision = output.precision();

    ConsoleReporter::write(output, 60.0, summary, roadResults);

    EXPECT_EQ(output.flags(), originalFlags);
    EXPECT_EQ(output.precision(), originalPrecision);

    const auto report = output.str();

    EXPECT_NE(report.find("TrafficSim Results\n\n"), std::string::npos);
    EXPECT_NE(report.find("Simulation Time:          60.00 s\n"), std::string::npos);
    EXPECT_NE(report.find("Vehicles Spawned:         5\n"), std::string::npos);
    EXPECT_NE(report.find("Vehicles Arrived:         4\n"), std::string::npos);
    EXPECT_NE(report.find("Average Travel Time:      12.50 s\n"), std::string::npos);
    EXPECT_NE(report.find("Average Waiting Time:     2.25 s\n"), std::string::npos);
    EXPECT_NE(report.find("Average Speed:            36.00 km/h\n"), std::string::npos);
    EXPECT_NE(report.find("Total Distance:           400.00 m\n"), std::string::npos);
    EXPECT_NE(report.find("Average Route Length:     100.00 m\n"), std::string::npos);
    EXPECT_NE(report.find("Peak Active Vehicles:     3\n"), std::string::npos);
    EXPECT_NE(report.find("Congested Roads:          1\n"), std::string::npos);
}

TEST(ConsoleReporterTests, RejectsInvalidSimulationTime)
{
    std::ostringstream output;
    const SimulationSummary summary;
    const std::vector<RoadResult> roadResults;

    EXPECT_THROW(ConsoleReporter::write(output, -1.0, summary, roadResults), std::invalid_argument);
    EXPECT_THROW(ConsoleReporter::write(output, std::numeric_limits<double>::infinity(), summary,
                                        roadResults),
                 std::invalid_argument);
}

} // namespace