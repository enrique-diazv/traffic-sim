#ifndef TRAFFICSIM_EXPERIMENTS_EXPERIMENT_PLAN_H
#define TRAFFICSIM_EXPERIMENTS_EXPERIMENT_PLAN_H

#include "trafficsim/experiments/ParameterSweep.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace trafficsim
{

struct ExperimentPlan
{
    std::filesystem::path scenarioPath;
    std::filesystem::path outputDirectory;
    std::size_t repetitions{1};
    std::uint64_t seedStride{1};
    std::vector<ParameterSweep> sweeps;

    void validate() const;
};

} // namespace trafficsim

#endif // TRAFFICSIM_EXPERIMENTS_EXPERIMENT_PLAN_H