#include "trafficsim/experiments/ParameterSweep.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace trafficsim
{
namespace
{

std::string_view parameterName(SweepParameter parameter)
{
    switch (parameter)
    {
    case SweepParameter::TimeStepSeconds:
        return "time_step_seconds";
    case SweepParameter::MaximumVehicles:
        return "maximum_vehicles";
    case SweepParameter::MaximumSpeedMetersPerSecond:
        return "maximum_speed_meters_per_second";
    case SweepParameter::AccelerationMetersPerSecondSquared:
        return "acceleration_meters_per_second_squared";
    case SweepParameter::DecelerationMetersPerSecondSquared:
        return "deceleration_meters_per_second_squared";
    case SweepParameter::MinimumFollowingDistanceMeters:
        return "minimum_following_distance_meters";
    case SweepParameter::ReactionTimeSeconds:
        return "reaction_time_seconds";
    case SweepParameter::ReroutingEvaluationIntervalSeconds:
        return "rerouting_evaluation_interval_seconds";
    case SweepParameter::ReroutingMinimumImprovementRatio:
        return "rerouting_minimum_improvement_ratio";
    case SweepParameter::ModeratePenaltyMultiplier:
        return "moderate_penalty_multiplier";
    case SweepParameter::CongestedPenaltyMultiplier:
        return "congested_penalty_multiplier";
    case SweepParameter::GridlockPenaltyMultiplier:
        return "gridlock_penalty_multiplier";
    }

    throw std::invalid_argument{"Unknown sweep parameter"};
}

double requireDouble(SweepParameter parameter, const SweepValue &value)
{
    if (const auto *number = std::get_if<double>(&value))
    {
        return *number;
    }

    throw std::invalid_argument{
        std::string{"Sweep parameter "} + std::string{parameterName(parameter)} +
            " requires a floating-point value",
    };
}

std::size_t requireSize(SweepParameter parameter, const SweepValue &value)
{
    if (const auto *number = std::get_if<std::size_t>(&value))
    {
        return *number;
    }

    throw std::invalid_argument{
        std::string{"Sweep parameter "} + std::string{parameterName(parameter)} +
            " requires an integer value",
    };
}

void applyValue(SimulationConfig &config, SweepParameter parameter, const SweepValue &value)
{
    switch (parameter)
    {
    case SweepParameter::TimeStepSeconds:
        config.timeStepSeconds = requireDouble(parameter, value);
        return;
    case SweepParameter::MaximumVehicles:
        config.maximumVehicles = requireSize(parameter, value);
        return;
    case SweepParameter::MaximumSpeedMetersPerSecond:
        config.defaultVehicleDynamics.maximumSpeedMetersPerSecond = requireDouble(parameter, value);
        return;
    case SweepParameter::AccelerationMetersPerSecondSquared:
        config.defaultVehicleDynamics.accelerationMetersPerSecondSquared =
            requireDouble(parameter, value);
        return;
    case SweepParameter::DecelerationMetersPerSecondSquared:
        config.defaultVehicleDynamics.decelerationMetersPerSecondSquared =
            requireDouble(parameter, value);
        return;
    case SweepParameter::MinimumFollowingDistanceMeters:
        config.minimumFollowingDistanceMeters = requireDouble(parameter, value);
        return;
    case SweepParameter::ReactionTimeSeconds:
        config.reactionTimeSeconds = requireDouble(parameter, value);
        return;
    case SweepParameter::ReroutingEvaluationIntervalSeconds:
        config.rerouting.evaluationIntervalSeconds = requireDouble(parameter, value);
        return;
    case SweepParameter::ReroutingMinimumImprovementRatio:
        config.rerouting.minimumImprovementRatio = requireDouble(parameter, value);
        return;
    case SweepParameter::ModeratePenaltyMultiplier:
        config.congestionCost.moderatePenaltyMultiplier = requireDouble(parameter, value);
        return;
    case SweepParameter::CongestedPenaltyMultiplier:
        config.congestionCost.congestedPenaltyMultiplier = requireDouble(parameter, value);
        return;
    case SweepParameter::GridlockPenaltyMultiplier:
        config.congestionCost.gridlockPenaltyMultiplier = requireDouble(parameter, value);
        return;
    }

    throw std::invalid_argument{"Unknown sweep parameter"};
}

std::string formatValue(const SweepValue &value)
{
    return std::visit(
        [](const auto numericValue)
        {
            std::ostringstream output;
            output.imbue(std::locale::classic());

            if constexpr (std::is_same_v<decltype(numericValue), double>)
            {
                output << std::setprecision(15);
            }

            output << numericValue;
            return output.str();
        },
        value);
}

} // namespace

std::vector<ExperimentVariant>
ParameterSweepGenerator::generate(const SimulationConfig &baseConfig,
                                  std::span<const ParameterSweep> sweeps)
{
    baseConfig.validate();

    if (sweeps.empty())
    {
        return {
            ExperimentVariant{
                .name = "baseline",
                .simulationConfig = baseConfig,
            },
        };
    }

    std::unordered_set<SweepParameter> usedParameters;
    usedParameters.reserve(sweeps.size());

    std::vector<ExperimentVariant> variants{
        ExperimentVariant{
            .name = "",
            .simulationConfig = baseConfig,
        },
    };

    for (const auto &sweep : sweeps)
    {
        if (sweep.values.empty())
        {
            throw std::invalid_argument{"Parameter sweep values must not be empty"};
        }

        if (!usedParameters.insert(sweep.parameter).second)
        {
            throw std::invalid_argument{"Parameter sweep parameters must be unique"};
        }

        std::vector<ExperimentVariant> nextVariants;

        if (variants.size() > nextVariants.max_size() / sweep.values.size())
        {
            throw std::length_error{"Parameter sweep produces too many variants"};
        }

        nextVariants.reserve(variants.size() * sweep.values.size());

        for (const auto &variant : variants)
        {
            for (const auto &value : sweep.values)
            {
                auto simulationConfig = variant.simulationConfig;
                applyValue(simulationConfig, sweep.parameter, value);

                auto variantName = variant.name;

                if (!variantName.empty())
                {
                    variantName += ';';
                }

                variantName += parameterName(sweep.parameter);
                variantName += '=';
                variantName += formatValue(value);

                nextVariants.push_back({
                    .name = std::move(variantName),
                    .simulationConfig = simulationConfig,
                });
            }
        }

        variants = std::move(nextVariants);
    }

    for (const auto &variant : variants)
    {
        variant.simulationConfig.validate();
    }

    return variants;
}

} // namespace trafficsim