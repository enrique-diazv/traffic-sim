#include "trafficsim/core/Simulation.h"
#include "trafficsim/io/ScenarioLoader.h"
#include "trafficsim/statistics/ConsoleReporter.h"
#include "trafficsim/statistics/CsvExporter.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <utility>

namespace
{

void runSimulation(const std::filesystem::path &scenarioPath)
{
    auto scenario = trafficsim::ScenarioLoader::loadFromFile(scenarioPath);

    trafficsim::Simulation simulation{
        scenario.config,
        std::move(scenario.roadNetwork),
        std::move(scenario.spawnSchedule),
        std::move(scenario.trafficManager),
    };

    simulation.run();

    const auto summary = simulation.statistics().summary();
    const auto vehicleResults = simulation.statistics().vehicleResults();
    const auto roadResults = simulation.statistics().roadResults();

    trafficsim::ConsoleReporter::write(std::cout, simulation.clock().currentTimeSeconds(), summary,
                                       roadResults);

    trafficsim::CsvExporter::exportToDirectory(std::filesystem::path{"results"}, summary,
                                               vehicleResults, roadResults);
}

} // namespace

// All exceptions are handled below; clang-tidy does not model the MSVC filesystem conversion.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char *argv[])
{
    try
    {
        if (argc > 2)
        {
            std::cerr << "Usage: trafficsim [scenario-file]\n";
            return 2;
        }

        const auto scenarioPath = argc == 2 ? std::filesystem::path{argv[1]}
                                            : std::filesystem::path{"scenarios/basic.json"};

        runSimulation(scenarioPath);
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
