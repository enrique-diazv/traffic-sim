#include "trafficsim/experiments/BatchExperimentRunner.h"
#include "trafficsim/experiments/ExperimentAggregator.h"
#include "trafficsim/experiments/ExperimentCsvExporter.h"
#include "trafficsim/experiments/ExperimentPlanLoader.h"
#include "trafficsim/experiments/ParameterSweep.h"
#include "trafficsim/io/ScenarioLoader.h"

#include <exception>
#include <filesystem>
#include <iostream>

namespace
{

void runExperiment(const std::filesystem::path &planPath)
{
    const auto plan = trafficsim::ExperimentPlanLoader::loadFromFile(planPath);
    const auto scenario = trafficsim::ScenarioLoader::loadFromFile(plan.scenarioPath);

    const trafficsim::BatchExperimentConfig batchConfig{
        .repetitions = plan.repetitions,
        .seedStride = plan.seedStride,
        .variants = trafficsim::ParameterSweepGenerator::generate(scenario.config, plan.sweeps),
    };

    const auto runResults = trafficsim::BatchExperimentRunner::run(scenario, batchConfig);
    const auto aggregatedResults = trafficsim::ExperimentAggregator::aggregate(runResults);

    trafficsim::ExperimentCsvExporter::exportToDirectory(plan.outputDirectory, runResults,
                                                         aggregatedResults);

    std::cout << "TrafficSim batch experiment completed\n"
              << "Variants: " << aggregatedResults.size() << '\n'
              << "Runs: " << runResults.size() << '\n'
              << "Output: " << std::filesystem::absolute(plan.outputDirectory).string() << '\n';
}

} // namespace

// All exceptions are handled below; clang-tidy does not model every filesystem exception.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char *argv[])
{
    try
    {
        if (argc > 2)
        {
            std::cerr << "Usage: trafficsim_experiments [experiment-plan]\n";
            return 2;
        }

        const auto planPath = argc == 2 ? std::filesystem::path{argv[1]}
                                        : std::filesystem::path{"experiments/basic_sweep.json"};

        runExperiment(planPath);
        return 0;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "TrafficSim experiment failed: " << exception.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "TrafficSim experiment failed with an unknown error\n";
        return 1;
    }
}