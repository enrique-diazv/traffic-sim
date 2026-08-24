#ifndef TRAFFICSIM_EXPERIMENTS_EXPERIMENT_PLAN_LOADER_H
#define TRAFFICSIM_EXPERIMENTS_EXPERIMENT_PLAN_LOADER_H

#include "trafficsim/experiments/ExperimentPlan.h"

#include <filesystem>
#include <string_view>

namespace trafficsim
{

class ExperimentPlanLoader final
{
  public:
    [[nodiscard]] static ExperimentPlan loadFromFile(const std::filesystem::path &filePath);

    [[nodiscard]] static ExperimentPlan
    loadFromJson(std::string_view jsonText, const std::filesystem::path &baseDirectory = {});
};

} // namespace trafficsim

#endif // TRAFFICSIM_EXPERIMENTS_EXPERIMENT_PLAN_LOADER_H