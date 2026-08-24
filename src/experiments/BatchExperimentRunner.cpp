#include "trafficsim/experiments/BatchExperimentRunner.h"

#include "trafficsim/core/Simulation.h"

#include <stdexcept>

namespace trafficsim
{

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

            results.push_back({
                .variantName = variant.name,
                .repetitionIndex = repetitionIndex,
                .randomSeed = simulationConfig.randomSeed,
                .totalReroutes = simulation.dynamicRoutingManager().totalReroutes(),
                .summary = simulation.statistics().summary(),
            });
        }
    }

    return results;
}

} // namespace trafficsim