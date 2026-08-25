#include "BenchmarkCsvExporter.h"
#include "PerformanceBenchmarkRunner.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace
{

trafficsim::benchmarking::BenchmarkConfig quickConfig()
{
    trafficsim::benchmarking::BenchmarkConfig config;
    config.vehicleCounts = {100U};
    config.repetitions = 1U;
    config.roadLookupOperationsPerVehicle = 10U;
    config.vehicleUpdateSteps = 2U;
    config.fullSimulationDurationSeconds = 1.0;
    return config;
}

void runBenchmarks(const std::filesystem::path &outputPath, bool quickMode)
{
    const auto config = quickMode ? quickConfig() : trafficsim::benchmarking::BenchmarkConfig{};

    const auto samples =
        trafficsim::benchmarking::PerformanceBenchmarkRunner::run(config, std::cout);

    trafficsim::benchmarking::BenchmarkCsvExporter::exportToFile(outputPath, samples);

    std::cout << "TrafficSim benchmarks completed\n"
              << "Samples: " << samples.size() << '\n'
              << "Output: " << std::filesystem::absolute(outputPath).string() << '\n';
}

} // namespace

// All exceptions are handled below; clang-tidy does not model every filesystem exception.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char *argv[])
{
    try
    {
        if (argc > 3)
        {
            std::cerr << "Usage: trafficsim_benchmarks "
                         "[output-file] [--quick]\n";
            return 2;
        }

        bool quickMode{false};
        auto outputPath = std::filesystem::path{
            "results/benchmarks/benchmark_samples.csv",
        };

        if (argc >= 2)
        {
            const std::string_view firstArgument{argv[1]};

            if (firstArgument == "--quick")
            {
                quickMode = true;
                outputPath = "results/benchmarks/benchmark_smoke.csv";
            }
            else
            {
                outputPath = std::filesystem::path{argv[1]};
            }
        }

        if (argc == 3)
        {
            const std::string_view secondArgument{argv[2]};

            if (secondArgument != "--quick")
            {
                std::cerr << "The second argument must be --quick\n";
                return 2;
            }

            quickMode = true;
        }

        runBenchmarks(outputPath, quickMode);
        return 0;
    }
    catch (const std::exception &exception)
    {
        std::cerr << "TrafficSim benchmarks failed: " << exception.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "TrafficSim benchmarks failed with an unknown error\n";
        return 1;
    }
}