#ifndef TRAFFICSIM_EXPERIMENTS_BATCH_EXPERIMENT_CONFIG_H
#define TRAFFICSIM_EXPERIMENTS_BATCH_EXPERIMENT_CONFIG_H

#include "trafficsim/core/SimulationConfig.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace trafficsim
{

struct ExperimentVariant
{
    std::string name;
    SimulationConfig simulationConfig;
};

struct BatchExperimentConfig
{
    std::size_t repetitions{1};
    std::uint64_t seedStride{1};
    std::vector<ExperimentVariant> variants;

    void validate() const;
};

} // namespace trafficsim

#endif // TRAFFICSIM_EXPERIMENTS_BATCH_EXPERIMENT_CONFIG_H