#ifndef TRAFFICSIM_EXPERIMENTS_PARAMETER_SWEEP_H
#define TRAFFICSIM_EXPERIMENTS_PARAMETER_SWEEP_H

#include "trafficsim/core/SimulationConfig.h"
#include "trafficsim/experiments/BatchExperimentConfig.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

namespace trafficsim
{

enum class SweepParameter : std::uint8_t
{
    TimeStepSeconds,
    MaximumVehicles,
    MaximumSpeedMetersPerSecond,
    AccelerationMetersPerSecondSquared,
    DecelerationMetersPerSecondSquared,
    MinimumFollowingDistanceMeters,
    ReactionTimeSeconds,
    ReroutingEvaluationIntervalSeconds,
    ReroutingMinimumImprovementRatio,
    ModeratePenaltyMultiplier,
    CongestedPenaltyMultiplier,
    GridlockPenaltyMultiplier,
};

using SweepValue = std::variant<double, std::size_t>;

struct ParameterSweep
{
    SweepParameter parameter;
    std::vector<SweepValue> values;
};

class ParameterSweepGenerator final
{
  public:
    [[nodiscard]] static std::vector<ExperimentVariant>
    generate(const SimulationConfig &baseConfig, std::span<const ParameterSweep> sweeps);
};

} // namespace trafficsim

#endif // TRAFFICSIM_EXPERIMENTS_PARAMETER_SWEEP_H