#include "trafficsim/experiments/BatchExperimentConfig.h"

#include <limits>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace trafficsim
{

void BatchExperimentConfig::validate() const
{
    if (repetitions == 0U)
    {
        throw std::invalid_argument{"Batch experiment repetitions must be positive"};
    }

    if (seedStride == 0U)
    {
        throw std::invalid_argument{"Batch experiment seed stride must be positive"};
    }

    if (variants.empty())
    {
        throw std::invalid_argument{"Batch experiment requires at least one variant"};
    }

    const auto finalRepetitionIndex = static_cast<std::uint64_t>(repetitions - 1U);
    const auto maximumSeed = std::numeric_limits<std::uint64_t>::max();

    if (finalRepetitionIndex > maximumSeed / seedStride)
    {
        throw std::invalid_argument{"Batch experiment seed sequence would overflow"};
    }

    const auto maximumSeedOffset = finalRepetitionIndex * seedStride;
    std::unordered_set<std::string_view> variantNames;
    variantNames.reserve(variants.size());

    for (const auto &variant : variants)
    {
        if (variant.name.empty())
        {
            throw std::invalid_argument{"Experiment variant name must not be empty"};
        }

        if (!variantNames.insert(variant.name).second)
        {
            throw std::invalid_argument{"Experiment variant names must be unique"};
        }

        if (variant.simulationConfig.randomSeed > maximumSeed - maximumSeedOffset)
        {
            throw std::invalid_argument{"Experiment variant seed sequence would overflow"};
        }

        variant.simulationConfig.validate();
    }
}

} // namespace trafficsim