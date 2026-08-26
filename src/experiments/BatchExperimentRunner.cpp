#include "trafficsim/experiments/BatchExperimentRunner.h"

#include "trafficsim/core/Simulation.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <thread>

#include <stdexcept>

namespace trafficsim
{

namespace
{

ExperimentRunResult executeRun(const Scenario &baseScenario, const BatchExperimentConfig &config,
                               const ExperimentVariant &variant, std::size_t repetitionIndex)
{
    auto simulationConfig = variant.simulationConfig;
    const auto seedOffset = static_cast<std::uint64_t>(repetitionIndex) * config.seedStride;

    simulationConfig.randomSeed += seedOffset;

    Simulation simulation{
        simulationConfig,
        baseScenario.roadNetwork,
        baseScenario.spawnSchedule,
        baseScenario.trafficManager,
    };

    simulation.run();

    return {
        .variantName = variant.name,
        .repetitionIndex = repetitionIndex,
        .randomSeed = simulationConfig.randomSeed,
        .totalReroutes = simulation.dynamicRoutingManager().totalReroutes(),
        .summary = simulation.statistics().summary(),
    };
}

} // namespace

std::vector<ExperimentRunResult> BatchExperimentRunner::run(const Scenario &baseScenario,
                                                            const BatchExperimentConfig &config)
{
    config.validate();

    std::vector<ExperimentRunResult> results;

    if (config.variants.size() > results.max_size() / config.repetitions)
    {
        throw std::length_error{"Batch experiment contains too many runs"};
    }

    results.reserve(config.variants.size() * config.repetitions);

    for (const auto &variant : config.variants)
    {
        for (std::size_t repetitionIndex = 0U; repetitionIndex < config.repetitions;
             ++repetitionIndex)
        {
            results.push_back(executeRun(baseScenario, config, variant, repetitionIndex));
        }
    }

    return results;
}

std::vector<ExperimentRunResult>
BatchExperimentRunner::runParallel(const Scenario &baseScenario,
                                   const BatchExperimentConfig &config, std::size_t workerCount)
{
    config.validate();

    if (workerCount == 0U)
    {
        throw std::invalid_argument{"Parallel batch worker count must be positive"};
    }

    if (workerCount == 1U)
    {
        return run(baseScenario, config);
    }

    std::vector<ExperimentRunResult> results;

    if (config.variants.size() > results.max_size() / config.repetitions)
    {
        throw std::length_error{"Batch experiment contains too many runs"};
    }

    const auto runCount = config.variants.size() * config.repetitions;
    results.resize(runCount);

    const auto activeWorkerCount = std::min(workerCount, runCount);
    std::atomic<std::size_t> nextRunIndex{0U};
    std::atomic<bool> failed{false};
    std::exception_ptr firstException;
    std::mutex exceptionMutex;

    {
        std::vector<std::jthread> workers;
        workers.reserve(activeWorkerCount);

        const auto executeNextRun = [&]()
        {
            while (!failed.load())
            {
                const auto runIndex = nextRunIndex.fetch_add(1U);

                if (runIndex >= runCount)
                {
                    return;
                }

                const auto variantIndex = runIndex / config.repetitions;
                const auto repetitionIndex = runIndex % config.repetitions;

                try
                {
                    results[runIndex] = executeRun(baseScenario, config,
                                                   config.variants[variantIndex], repetitionIndex);
                }
                catch (...)
                {
                    {
                        const std::scoped_lock lock{exceptionMutex};

                        if (firstException == nullptr)
                        {
                            firstException = std::current_exception();
                        }
                    }

                    failed.store(true);
                    return;
                }
            }
        };

        for (std::size_t workerIndex = 0U; workerIndex < activeWorkerCount; ++workerIndex)
        {
            workers.emplace_back(executeNextRun);
        }
    }

    if (firstException != nullptr)
    {
        std::rethrow_exception(firstException);
    }

    return results;
}

} // namespace trafficsim