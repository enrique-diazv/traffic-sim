#ifndef TRAFFICSIM_EXPERIMENTS_BATCH_EXPERIMENT_RUNNER_H
#define TRAFFICSIM_EXPERIMENTS_BATCH_EXPERIMENT_RUNNER_H

#include "trafficsim/experiments/BatchExperimentConfig.h"
#include "trafficsim/io/Scenario.h"
#include "trafficsim/statistics/StatisticsTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace trafficsim
{

struct ExperimentRunResult
{
    std::string variantName;
    std::size_t repetitionIndex{};
    std::uint64_t randomSeed{};
    std::size_t totalReroutes{};
    SimulationSummary summary;
};

class BatchExperimentRunner final
{
  public:
    [[nodiscard]] static std::vector<ExperimentRunResult> run(const Scenario &baseScenario,
                                                              const BatchExperimentConfig &config);
    [[nodiscard]] static std::vector<ExperimentRunResult>
    runParallel(const Scenario &baseScenario, const BatchExperimentConfig &config,
                std::size_t workerCount);
};

} // namespace trafficsim

#endif // TRAFFICSIM_EXPERIMENTS_BATCH_EXPERIMENT_RUNNER_H