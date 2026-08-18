#include "trafficsim/core/Simulation.h"
#include "trafficsim/statistics/ConsoleReporter.h"
#include "trafficsim/statistics/CsvExporter.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

namespace
{

trafficsim::RoadNetwork createDemoNetwork()
{
    using trafficsim::Intersection;
    using trafficsim::Road;
    using trafficsim::RoadNetwork;
    using trafficsim::RoadProperties;

    RoadNetwork network;

    network.addIntersection(Intersection{1, {.x = 0.0, .y = 0.0}});
    network.addIntersection(Intersection{2, {.x = 30.0, .y = 0.0}});
    network.addIntersection(Intersection{3, {.x = 60.0, .y = 0.0}});

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 30.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 10,
        },
    });

    network.addRoad(Road{
        20,
        RoadProperties{
            .origin = 2,
            .destination = 3,
            .lengthMeters = 30.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 10,
        },
    });

    return network;
}

trafficsim::SimulationConfig createDemoConfig()
{
    trafficsim::SimulationConfig config;

    config.durationSeconds = 15.0;
    config.timeStepSeconds = 0.1;
    config.maximumVehicles = 10;
    config.defaultVehicleDynamics = trafficsim::VehicleDynamics{
        .maximumSpeedMetersPerSecond = 10.0,
        .accelerationMetersPerSecondSquared = 5.0,
        .decelerationMetersPerSecondSquared = 5.0,
    };

    return config;
}

} // namespace

// All exceptions are handled below; clang-tidy does not model the MSVC filesystem conversion.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main()
{
    try
    {
        const std::vector<trafficsim::VehicleSpawnRequest> schedule{
            {
                .spawnTimeSeconds = 0.0,
                .origin = 1,
                .destination = 3,
            },
            {
                .spawnTimeSeconds = 1.0,
                .origin = 1,
                .destination = 3,
            },
            {
                .spawnTimeSeconds = 2.0,
                .origin = 1,
                .destination = 3,
            },
        };

        trafficsim::Simulation simulation{
            createDemoConfig(),
            createDemoNetwork(),
            schedule,
        };

        simulation.run();

        const auto summary = simulation.statistics().summary();
        const auto vehicleResults = simulation.statistics().vehicleResults();
        const auto roadResults = simulation.statistics().roadResults();

        trafficsim::ConsoleReporter::write(std::cout, simulation.clock().currentTimeSeconds(),
                                           summary, roadResults);

        trafficsim::CsvExporter::exportToDirectory(std::filesystem::path{"results"}, summary,
                                                   vehicleResults, roadResults);

        return 0;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "TrafficSim failed: " << exception.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "TrafficSim failed with an unknown error\n";
        return 1;
    }
}
