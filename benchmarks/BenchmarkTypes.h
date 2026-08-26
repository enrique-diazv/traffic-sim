#ifndef TRAFFICSIM_BENCHMARKS_BENCHMARK_TYPES_H
#define TRAFFICSIM_BENCHMARKS_BENCHMARK_TYPES_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace trafficsim::benchmarking
{

enum class BenchmarkKind : std::uint8_t
{
    RoadLookup,
    Routing,
    VehicleUpdate,
    Statistics,
    FullSimulation,
    BatchSequential,
    BatchParallel,
};

struct BenchmarkConfig
{
    std::vector<std::size_t> vehicleCounts{
        100U,
        1'000U,
        5'000U,
        10'000U,
    };

    std::size_t repetitions{3U};
    std::size_t roadLookupOperationsPerVehicle{100U};
    std::size_t vehicleUpdateSteps{10U};
    std::size_t batchRunCount{8U};
    std::size_t parallelWorkerCount{4U};
    double simulationTimeStepSeconds{0.1};
    double fullSimulationDurationSeconds{5.0};

    void validate() const;
};

struct BenchmarkSample
{
    BenchmarkKind kind{};
    std::size_t vehicleCount{};
    std::size_t repetitionIndex{};
    std::size_t operationCount{};
    double wallTimeMilliseconds{};
    double cpuTimeMilliseconds{};
    double cpuUtilizationPercent{};
    double simulatedSeconds{};
    double simulatedSecondsPerRealSecond{};
};

[[nodiscard]] std::string_view benchmarkName(BenchmarkKind kind);

} // namespace trafficsim::benchmarking

#endif // TRAFFICSIM_BENCHMARKS_BENCHMARK_TYPES_H