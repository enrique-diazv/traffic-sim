#include "trafficsim/experiments/ExperimentPlan.h"

#include <stdexcept>
#include <unordered_set>

namespace trafficsim
{

void ExperimentPlan::validate() const
{
    if (scenarioPath.empty())
    {
        throw std::invalid_argument{"Experiment scenario path must not be empty"};
    }

    if (outputDirectory.empty())
    {
        throw std::invalid_argument{"Experiment output directory must not be empty"};
    }

    if (repetitions == 0U)
    {
        throw std::invalid_argument{"Experiment repetitions must be positive"};
    }

    if (seedStride == 0U)
    {
        throw std::invalid_argument{"Experiment seed stride must be positive"};
    }

    std::unordered_set<SweepParameter> usedParameters;
    usedParameters.reserve(sweeps.size());

    for (const auto &sweep : sweeps)
    {
        if (sweep.values.empty())
        {
            throw std::invalid_argument{"Experiment sweep values must not be empty"};
        }

        if (!usedParameters.insert(sweep.parameter).second)
        {
            throw std::invalid_argument{"Experiment sweep parameters must be unique"};
        }
    }
}

} // namespace trafficsim