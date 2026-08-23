#include "SimulationVisualizer.h"

#include "trafficsim/core/Simulation.h"
#include "trafficsim/io/ScenarioLoader.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <utility>

namespace
{

void runVisualizer(const std::filesystem::path &scenarioPath,
                   trafficsim::visualization::VisualizationConfig visualizationConfig)
{
    auto scenario = trafficsim::ScenarioLoader::loadFromFile(scenarioPath);

    trafficsim::Simulation simulation{
        scenario.config,
        std::move(scenario.roadNetwork),
        std::move(scenario.spawnSchedule),
        std::move(scenario.trafficManager),
    };

    trafficsim::visualization::SimulationVisualizer visualizer{
        simulation,
        std::move(visualizationConfig),
    };
    visualizer.run();
}

} // namespace

// All exceptions are handled below; clang-tidy does not model every SFML or filesystem exception.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char *argv[])
{
    try
    {
        if (argc > 2)
        {
            std::cerr << "Usage: trafficsim_visualizer [scenario-file]\n";
            return 2;
        }

        const auto executableDirectory =
            std::filesystem::absolute(std::filesystem::path{argv[0]}).parent_path();

        const auto scenarioPath = argc == 2 ? std::filesystem::path{argv[1]}
                                            : executableDirectory / "scenarios" / "basic.json";

        trafficsim::visualization::VisualizationConfig visualizationConfig{};
        visualizationConfig.fontPath =
            executableDirectory / "assets" / "fonts" / "RobotoMono-Regular.ttf";

        runVisualizer(scenarioPath, std::move(visualizationConfig));
        return 0;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "TrafficSim visualizer failed: " << exception.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "TrafficSim visualizer failed with an unknown error\n";
        return 1;
    }
}