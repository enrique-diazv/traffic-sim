#include "PerformanceBenchmarkRunner.h"

#include "BenchmarkFixtures.h"
#include "BenchmarkTimer.h"

#include "trafficsim/experiments/BatchExperimentRunner.h"
#include "trafficsim/routing/DijkstraRoutePlanner.h"
#include "trafficsim/statistics/StatisticsCollector.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace trafficsim::benchmarking
{
namespace
{

constexpr std::size_t routingRoadCount{100U};
std::atomic<std::uint64_t> benchmarkSink{};

struct SampleDescriptor
{
    BenchmarkKind kind;
    std::size_t vehicleCount;
    std::size_t repetitionIndex;
    std::size_t operationCount;
    double simulatedSeconds;
};

BenchmarkSample makeSample(const SampleDescriptor &descriptor, const TimingResult &timing)
{
    const auto wallTimeSeconds = timing.wallTimeMilliseconds / 1000.0;

    return {
        .kind = descriptor.kind,
        .vehicleCount = descriptor.vehicleCount,
        .repetitionIndex = descriptor.repetitionIndex,
        .operationCount = descriptor.operationCount,
        .wallTimeMilliseconds = timing.wallTimeMilliseconds,
        .cpuTimeMilliseconds = timing.cpuTimeMilliseconds,
        .cpuUtilizationPercent = timing.cpuUtilizationPercent,
        .simulatedSeconds = descriptor.simulatedSeconds,
        .simulatedSecondsPerRealSecond = descriptor.simulatedSeconds > 0.0 && wallTimeSeconds > 0.0
                                             ? descriptor.simulatedSeconds / wallTimeSeconds
                                             : 0.0,
    };
}

void writeProgress(std::ostream &output, const SampleDescriptor &descriptor)
{
    output << "  " << benchmarkName(descriptor.kind) << " | vehicles=" << descriptor.vehicleCount
           << " | repetition=" << descriptor.repetitionIndex + 1U << '\n';
    output.flush();
}

BenchmarkSample benchmarkRoadLookup(const BenchmarkConfig &config, std::size_t vehicleCount,
                                    std::size_t repetitionIndex)
{
    const auto network = createLinearNetwork(routingRoadCount, 100.0);
    const auto roadIds = network.roadIds();
    const auto operationCount = vehicleCount * config.roadLookupOperationsPerVehicle;

    const auto timing = BenchmarkTimer::measure(
        [&network, &roadIds, operationCount]()
        {
            std::uint64_t checksum{};

            for (std::size_t index = 0U; index < operationCount; ++index)
            {
                const auto roadId = roadIds[index % roadIds.size()];
                checksum += network.getRoad(roadId).capacity();
            }

            benchmarkSink.store(checksum, std::memory_order_relaxed);
        });

    return makeSample(
        SampleDescriptor{
            .kind = BenchmarkKind::RoadLookup,
            .vehicleCount = vehicleCount,
            .repetitionIndex = repetitionIndex,
            .operationCount = operationCount,
            .simulatedSeconds = 0.0,
        },
        timing);
}

BenchmarkSample benchmarkRouting(std::size_t vehicleCount, std::size_t repetitionIndex)
{
    const auto network = createLinearNetwork(routingRoadCount, 100.0);
    const DijkstraRoutePlanner routePlanner;

    const auto timing = BenchmarkTimer::measure(
        [&network, &routePlanner, vehicleCount]()
        {
            std::uint64_t checksum{};

            for (std::size_t index = 0U; index < vehicleCount; ++index)
            {
                static_cast<void>(index);

                const auto route = routePlanner.calculateRoute(
                    network, 1U, static_cast<IntersectionId>(routingRoadCount + 1U));

                if (!route.has_value())
                {
                    throw std::logic_error{
                        "Benchmark routing network became unreachable",
                    };
                }

                checksum += route->segmentCount();
            }

            benchmarkSink.store(checksum, std::memory_order_relaxed);
        });

    return makeSample(
        SampleDescriptor{
            .kind = BenchmarkKind::Routing,
            .vehicleCount = vehicleCount,
            .repetitionIndex = repetitionIndex,
            .operationCount = vehicleCount,
            .simulatedSeconds = 0.0,
        },
        timing);
}

BenchmarkSample benchmarkVehicleUpdate(const BenchmarkConfig &config, std::size_t vehicleCount,
                                       std::size_t repetitionIndex)
{
    const auto network = createLinearNetwork(1U, 1'000'000.0);
    auto vehicleManager = createVehicleManager(vehicleCount, network);

    const auto timing = BenchmarkTimer::measure(
        [&config, &network, &vehicleManager]()
        {
            for (std::size_t step = 0U; step < config.vehicleUpdateSteps; ++step)
            {
                vehicleManager.update(config.simulationTimeStepSeconds, network);
            }

            benchmarkSink.store(vehicleManager.vehicleCount(), std::memory_order_relaxed);
        });

    const auto simulatedSeconds =
        static_cast<double>(config.vehicleUpdateSteps) * config.simulationTimeStepSeconds;

    return makeSample(
        SampleDescriptor{
            .kind = BenchmarkKind::VehicleUpdate,
            .vehicleCount = vehicleCount,
            .repetitionIndex = repetitionIndex,
            .operationCount = vehicleCount * config.vehicleUpdateSteps,
            .simulatedSeconds = simulatedSeconds,
        },
        timing);
}

BenchmarkSample benchmarkStatistics(const BenchmarkConfig &config, std::size_t vehicleCount,
                                    std::size_t repetitionIndex)
{
    const auto roadMetrics = createRoadMetrics(vehicleCount);
    StatisticsCollector collector;

    const auto timing = BenchmarkTimer::measure(
        [&collector, &config, &roadMetrics]()
        {
            collector.observeRoads(config.simulationTimeStepSeconds, roadMetrics);

            const auto results = collector.roadResults();

            benchmarkSink.store(results.size(), std::memory_order_relaxed);
        });

    return makeSample(
        SampleDescriptor{
            .kind = BenchmarkKind::Statistics,
            .vehicleCount = vehicleCount,
            .repetitionIndex = repetitionIndex,
            .operationCount = vehicleCount,
            .simulatedSeconds = 0.0,
        },
        timing);
}

BenchmarkSample benchmarkFullSimulation(const BenchmarkConfig &config, std::size_t vehicleCount,
                                        std::size_t repetitionIndex)
{
    auto simulation = createSimulation(vehicleCount, config.simulationTimeStepSeconds,
                                       config.fullSimulationDurationSeconds);

    const auto timing = BenchmarkTimer::measure(
        [&simulation]()
        {
            simulation.run();

            benchmarkSink.store(simulation.totalSpawnedVehicles(), std::memory_order_relaxed);
        });

    const auto stepCount = static_cast<std::size_t>(
        std::ceil(config.fullSimulationDurationSeconds / config.simulationTimeStepSeconds));

    if (vehicleCount > std::numeric_limits<std::size_t>::max() / stepCount)
    {
        throw std::overflow_error{
            "Full simulation benchmark operation count overflow",
        };
    }

    return makeSample(
        SampleDescriptor{
            .kind = BenchmarkKind::FullSimulation,
            .vehicleCount = vehicleCount,
            .repetitionIndex = repetitionIndex,
            .operationCount = vehicleCount * stepCount,
            .simulatedSeconds = config.fullSimulationDurationSeconds,
        },
        timing);
}

std::size_t calculateBatchOperationCount(const BenchmarkConfig &config, std::size_t vehicleCount)
{
    const auto stepCount = static_cast<std::size_t>(
        std::ceil(config.fullSimulationDurationSeconds / config.simulationTimeStepSeconds));

    if (vehicleCount > std::numeric_limits<std::size_t>::max() / stepCount)
    {
        throw std::overflow_error{
            "Batch benchmark operation count overflow",
        };
    }

    const auto operationsPerRun = vehicleCount * stepCount;

    if (operationsPerRun > std::numeric_limits<std::size_t>::max() / config.batchRunCount)
    {
        throw std::overflow_error{
            "Batch benchmark operation count overflow",
        };
    }

    return operationsPerRun * config.batchRunCount;
}

BenchmarkSample benchmarkBatch(const BenchmarkConfig &config, std::size_t vehicleCount,
                               std::size_t repetitionIndex, BenchmarkKind kind)
{
    if (kind != BenchmarkKind::BatchSequential && kind != BenchmarkKind::BatchParallel)
    {
        throw std::invalid_argument{"Invalid batch benchmark kind"};
    }

    const auto scenario = createBenchmarkScenario(vehicleCount, config.simulationTimeStepSeconds,
                                                  config.fullSimulationDurationSeconds);
    const auto batchConfig = createBatchExperimentConfig(scenario, config.batchRunCount);

    const auto timing = BenchmarkTimer::measure(
        [&config, &scenario, &batchConfig, kind]()
        {
            const auto results = kind == BenchmarkKind::BatchParallel
                                     ? BatchExperimentRunner::runParallel(
                                           scenario, batchConfig, config.parallelWorkerCount)
                                     : BatchExperimentRunner::run(scenario, batchConfig);

            benchmarkSink.store(results.size(), std::memory_order_relaxed);
        });

    return makeSample(
        SampleDescriptor{
            .kind = kind,
            .vehicleCount = vehicleCount,
            .repetitionIndex = repetitionIndex,
            .operationCount = calculateBatchOperationCount(config, vehicleCount),
            .simulatedSeconds =
                config.fullSimulationDurationSeconds * static_cast<double>(config.batchRunCount),
        },
        timing);
}

} // namespace

std::vector<BenchmarkSample> PerformanceBenchmarkRunner::run(const BenchmarkConfig &config,
                                                             std::ostream &progressOutput)
{
    config.validate();

    constexpr std::size_t benchmarkKindCount{7U};

    if (config.vehicleCounts.size() >
        std::numeric_limits<std::size_t>::max() / config.repetitions / benchmarkKindCount)
    {
        throw std::length_error{"Benchmark sample count overflow"};
    }

    std::vector<BenchmarkSample> samples;
    samples.reserve(config.vehicleCounts.size() * config.repetitions * benchmarkKindCount);

    for (const auto vehicleCount : config.vehicleCounts)
    {
        for (std::size_t repetitionIndex = 0U; repetitionIndex < config.repetitions;
             ++repetitionIndex)
        {
            const SampleDescriptor roadLookupDescriptor{
                .kind = BenchmarkKind::RoadLookup,
                .vehicleCount = vehicleCount,
                .repetitionIndex = repetitionIndex,
                .operationCount = vehicleCount * config.roadLookupOperationsPerVehicle,
                .simulatedSeconds = 0.0,
            };
            writeProgress(progressOutput, roadLookupDescriptor);
            samples.push_back(benchmarkRoadLookup(config, vehicleCount, repetitionIndex));

            const SampleDescriptor routingDescriptor{
                .kind = BenchmarkKind::Routing,
                .vehicleCount = vehicleCount,
                .repetitionIndex = repetitionIndex,
                .operationCount = vehicleCount,
                .simulatedSeconds = 0.0,
            };
            writeProgress(progressOutput, routingDescriptor);
            samples.push_back(benchmarkRouting(vehicleCount, repetitionIndex));

            const SampleDescriptor vehicleUpdateDescriptor{
                .kind = BenchmarkKind::VehicleUpdate,
                .vehicleCount = vehicleCount,
                .repetitionIndex = repetitionIndex,
                .operationCount = vehicleCount * config.vehicleUpdateSteps,
                .simulatedSeconds = static_cast<double>(config.vehicleUpdateSteps) *
                                    config.simulationTimeStepSeconds,
            };
            writeProgress(progressOutput, vehicleUpdateDescriptor);
            samples.push_back(benchmarkVehicleUpdate(config, vehicleCount, repetitionIndex));

            const SampleDescriptor statisticsDescriptor{
                .kind = BenchmarkKind::Statistics,
                .vehicleCount = vehicleCount,
                .repetitionIndex = repetitionIndex,
                .operationCount = vehicleCount,
                .simulatedSeconds = 0.0,
            };
            writeProgress(progressOutput, statisticsDescriptor);
            samples.push_back(benchmarkStatistics(config, vehicleCount, repetitionIndex));

            const SampleDescriptor fullSimulationDescriptor{
                .kind = BenchmarkKind::FullSimulation,
                .vehicleCount = vehicleCount,
                .repetitionIndex = repetitionIndex,
                .operationCount = vehicleCount,
                .simulatedSeconds = config.fullSimulationDurationSeconds,
            };
            writeProgress(progressOutput, fullSimulationDescriptor);
            samples.push_back(benchmarkFullSimulation(config, vehicleCount, repetitionIndex));
            const auto batchOperationCount = calculateBatchOperationCount(config, vehicleCount);
            const auto batchSimulatedSeconds =
                config.fullSimulationDurationSeconds * static_cast<double>(config.batchRunCount);

            const SampleDescriptor sequentialBatchDescriptor{
                .kind = BenchmarkKind::BatchSequential,
                .vehicleCount = vehicleCount,
                .repetitionIndex = repetitionIndex,
                .operationCount = batchOperationCount,
                .simulatedSeconds = batchSimulatedSeconds,
            };
            writeProgress(progressOutput, sequentialBatchDescriptor);
            samples.push_back(benchmarkBatch(config, vehicleCount, repetitionIndex,
                                             BenchmarkKind::BatchSequential));

            const SampleDescriptor parallelBatchDescriptor{
                .kind = BenchmarkKind::BatchParallel,
                .vehicleCount = vehicleCount,
                .repetitionIndex = repetitionIndex,
                .operationCount = batchOperationCount,
                .simulatedSeconds = batchSimulatedSeconds,
            };
            writeProgress(progressOutput, parallelBatchDescriptor);
            samples.push_back(benchmarkBatch(config, vehicleCount, repetitionIndex,
                                             BenchmarkKind::BatchParallel));
        }
    }

    return samples;
}

} // namespace trafficsim::benchmarking