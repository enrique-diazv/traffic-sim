#include "trafficsim/experiments/ExperimentPlanLoader.h"

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace trafficsim
{
namespace
{

using Json = nlohmann::json;

constexpr std::array<std::pair<std::string_view, SweepParameter>, 12> parameterMappings{
    std::pair{"time_step_seconds", SweepParameter::TimeStepSeconds},
    std::pair{"maximum_vehicles", SweepParameter::MaximumVehicles},
    std::pair{"maximum_speed_meters_per_second", SweepParameter::MaximumSpeedMetersPerSecond},
    std::pair{"acceleration_meters_per_second_squared",
              SweepParameter::AccelerationMetersPerSecondSquared},
    std::pair{"deceleration_meters_per_second_squared",
              SweepParameter::DecelerationMetersPerSecondSquared},
    std::pair{"minimum_following_distance_meters", SweepParameter::MinimumFollowingDistanceMeters},
    std::pair{"reaction_time_seconds", SweepParameter::ReactionTimeSeconds},
    std::pair{"rerouting_evaluation_interval_seconds",
              SweepParameter::ReroutingEvaluationIntervalSeconds},
    std::pair{"rerouting_minimum_improvement_ratio",
              SweepParameter::ReroutingMinimumImprovementRatio},
    std::pair{"moderate_penalty_multiplier", SweepParameter::ModeratePenaltyMultiplier},
    std::pair{"congested_penalty_multiplier", SweepParameter::CongestedPenaltyMultiplier},
    std::pair{"gridlock_penalty_multiplier", SweepParameter::GridlockPenaltyMultiplier},
};

const Json &requiredMember(const Json &object, std::string_view memberName)
{
    const auto memberIterator = object.find(memberName);

    if (memberIterator == object.end())
    {
        throw std::invalid_argument{
            "Missing experiment member: " + std::string{memberName},
        };
    }

    return *memberIterator;
}

std::string requiredString(const Json &object, std::string_view memberName)
{
    const auto &value = requiredMember(object, memberName);

    if (!value.is_string())
    {
        throw std::invalid_argument{
            "Experiment member must be a string: " + std::string{memberName},
        };
    }

    return value.get<std::string>();
}

std::uint64_t requiredUnsigned(const Json &object, std::string_view memberName)
{
    const auto &value = requiredMember(object, memberName);

    if (!value.is_number_unsigned())
    {
        throw std::invalid_argument{
            "Experiment member must be an unsigned integer: " + std::string{memberName},
        };
    }

    return value.get<std::uint64_t>();
}

std::size_t requiredSize(const Json &object, std::string_view memberName)
{
    const auto value = requiredUnsigned(object, memberName);

    if (value > std::numeric_limits<std::size_t>::max())
    {
        throw std::invalid_argument{
            "Experiment member exceeds the supported size: " + std::string{memberName},
        };
    }

    return static_cast<std::size_t>(value);
}

SweepParameter parseParameter(std::string_view parameterName)
{
    for (const auto &[name, parameter] : parameterMappings)
    {
        if (name == parameterName)
        {
            return parameter;
        }
    }

    throw std::invalid_argument{
        "Unknown experiment sweep parameter: " + std::string{parameterName},
    };
}

SweepValue parseValue(SweepParameter parameter, const Json &value)
{
    if (parameter == SweepParameter::MaximumVehicles)
    {
        if (!value.is_number_unsigned())
        {
            throw std::invalid_argument{
                "maximum_vehicles sweep values must be unsigned integers",
            };
        }

        const auto maximumVehicles = value.get<std::uint64_t>();

        if (maximumVehicles > std::numeric_limits<std::size_t>::max())
        {
            throw std::invalid_argument{
                "maximum_vehicles sweep value exceeds the supported size",
            };
        }

        return static_cast<std::size_t>(maximumVehicles);
    }

    if (!value.is_number())
    {
        throw std::invalid_argument{
            "Experiment sweep values must be numeric",
        };
    }

    const auto numericValue = value.get<double>();

    if (!std::isfinite(numericValue))
    {
        throw std::invalid_argument{
            "Experiment sweep values must be finite",
        };
    }

    return numericValue;
}

std::vector<ParameterSweep> parseSweeps(const Json &document)
{
    const auto sweepsIterator = document.find("sweeps");

    if (sweepsIterator == document.end())
    {
        return {};
    }

    if (!sweepsIterator->is_array())
    {
        throw std::invalid_argument{
            "Experiment sweeps must be an array",
        };
    }

    std::vector<ParameterSweep> sweeps;
    sweeps.reserve(sweepsIterator->size());

    for (const auto &sweepJson : *sweepsIterator)
    {
        if (!sweepJson.is_object())
        {
            throw std::invalid_argument{
                "Each experiment sweep must be an object",
            };
        }

        const auto parameter = parseParameter(requiredString(sweepJson, "parameter"));
        const auto &valuesJson = requiredMember(sweepJson, "values");

        if (!valuesJson.is_array())
        {
            throw std::invalid_argument{
                "Experiment sweep values must be an array",
            };
        }

        std::vector<SweepValue> values;
        values.reserve(valuesJson.size());

        for (const auto &valueJson : valuesJson)
        {
            values.push_back(parseValue(parameter, valueJson));
        }

        sweeps.push_back({
            .parameter = parameter,
            .values = std::move(values),
        });
    }

    return sweeps;
}

std::filesystem::path resolvePath(const std::filesystem::path &baseDirectory,
                                  const std::string &pathText)
{
    const std::filesystem::path path{pathText};

    if (path.is_absolute() || baseDirectory.empty())
    {
        return path.lexically_normal();
    }

    return (baseDirectory / path).lexically_normal();
}

ExperimentPlan parsePlan(const Json &document, const std::filesystem::path &baseDirectory)
{
    if (!document.is_object())
    {
        throw std::invalid_argument{
            "Experiment JSON root must be an object",
        };
    }

    const auto scenarioPath = resolvePath(baseDirectory, requiredString(document, "scenario"));

    const auto outputDirectory =
        document.contains("output_directory")
            ? resolvePath(baseDirectory, requiredString(document, "output_directory"))
            : resolvePath(baseDirectory, "results/experiments");

    const auto seedStride = document.contains("seed_stride")
                                ? requiredUnsigned(document, "seed_stride")
                                : std::uint64_t{1U};

    ExperimentPlan plan{
        .scenarioPath = scenarioPath,
        .outputDirectory = outputDirectory,
        .repetitions = requiredSize(document, "repetitions"),
        .seedStride = seedStride,
        .sweeps = parseSweeps(document),
    };

    plan.validate();
    return plan;
}

} // namespace

ExperimentPlan ExperimentPlanLoader::loadFromFile(const std::filesystem::path &filePath)
{
    if (filePath.empty())
    {
        throw std::invalid_argument{
            "Experiment plan file path must not be empty",
        };
    }

    std::ifstream input{filePath};

    if (!input.is_open())
    {
        throw std::runtime_error{
            "Could not open experiment plan file: " + filePath.string(),
        };
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (input.bad())
    {
        throw std::runtime_error{
            "Could not read experiment plan file: " + filePath.string(),
        };
    }

    return loadFromJson(buffer.str(), filePath.parent_path());
}

ExperimentPlan ExperimentPlanLoader::loadFromJson(std::string_view jsonText,
                                                  const std::filesystem::path &baseDirectory)
{
    try
    {
        const auto document = Json::parse(jsonText.begin(), jsonText.end());
        return parsePlan(document, baseDirectory);
    }
    catch (const nlohmann::json::exception &exception)
    {
        throw std::invalid_argument{
            "Invalid experiment JSON: " + std::string{exception.what()},
        };
    }
}

} // namespace trafficsim