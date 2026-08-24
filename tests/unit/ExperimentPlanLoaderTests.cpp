#include "trafficsim/experiments/ExperimentPlanLoader.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <variant>

namespace
{

using trafficsim::ExperimentPlanLoader;
using trafficsim::SweepParameter;

TEST(ExperimentPlanLoaderTests, LoadsPlanAndResolvesRelativePaths)
{
    constexpr std::string_view jsonText = R"json(
        {
            "scenario": "../scenarios/basic.json",
            "output_directory": "../results/comparison",
            "repetitions": 3,
            "seed_stride": 100,
            "sweeps": [
                {
                    "parameter": "time_step_seconds",
                    "values": [0.1, 0.2]
                },
                {
                    "parameter": "maximum_vehicles",
                    "values": [10, 20]
                }
            ]
        }
    )json";

    const auto plan = ExperimentPlanLoader::loadFromJson(jsonText, std::filesystem::path{"plans"});

    EXPECT_EQ(plan.scenarioPath, std::filesystem::path{"scenarios/basic.json"});
    EXPECT_EQ(plan.outputDirectory, std::filesystem::path{"results/comparison"});
    EXPECT_EQ(plan.repetitions, 3U);
    EXPECT_EQ(plan.seedStride, 100U);

    ASSERT_EQ(plan.sweeps.size(), 2U);

    EXPECT_EQ(plan.sweeps[0].parameter, SweepParameter::TimeStepSeconds);
    ASSERT_EQ(plan.sweeps[0].values.size(), 2U);
    EXPECT_DOUBLE_EQ(std::get<double>(plan.sweeps[0].values[0]), 0.1);
    EXPECT_DOUBLE_EQ(std::get<double>(plan.sweeps[0].values[1]), 0.2);

    EXPECT_EQ(plan.sweeps[1].parameter, SweepParameter::MaximumVehicles);
    ASSERT_EQ(plan.sweeps[1].values.size(), 2U);
    EXPECT_EQ(std::get<std::size_t>(plan.sweeps[1].values[0]), 10U);
    EXPECT_EQ(std::get<std::size_t>(plan.sweeps[1].values[1]), 20U);
}

TEST(ExperimentPlanLoaderTests, AppliesOptionalDefaults)
{
    constexpr std::string_view jsonText = R"json(
        {
            "scenario": "basic.json",
            "repetitions": 1
        }
    )json";

    const auto plan =
        ExperimentPlanLoader::loadFromJson(jsonText, std::filesystem::path{"workspace"});

    EXPECT_EQ(plan.scenarioPath, std::filesystem::path{"workspace/basic.json"});
    EXPECT_EQ(plan.outputDirectory, std::filesystem::path{"workspace/results/experiments"});
    EXPECT_EQ(plan.repetitions, 1U);
    EXPECT_EQ(plan.seedStride, 1U);
    EXPECT_TRUE(plan.sweeps.empty());
}

TEST(ExperimentPlanLoaderTests, RejectsMalformedOrIncompleteDocuments)
{
    EXPECT_THROW(static_cast<void>(ExperimentPlanLoader::loadFromJson("{")), std::invalid_argument);

    EXPECT_THROW(static_cast<void>(ExperimentPlanLoader::loadFromJson("[]")),
                 std::invalid_argument);

    EXPECT_THROW(
        static_cast<void>(ExperimentPlanLoader::loadFromJson(R"json({"repetitions": 1})json")),
        std::invalid_argument);
}

TEST(ExperimentPlanLoaderTests, RejectsUnknownOrIncorrectlyTypedSweeps)
{
    constexpr std::string_view unknownParameter = R"json(
        {
            "scenario": "basic.json",
            "repetitions": 1,
            "sweeps": [
                {
                    "parameter": "unknown_parameter",
                    "values": [1.0]
                }
            ]
        }
    )json";

    EXPECT_THROW(static_cast<void>(ExperimentPlanLoader::loadFromJson(unknownParameter)),
                 std::invalid_argument);

    constexpr std::string_view incorrectMaximumVehicles = R"json(
        {
            "scenario": "basic.json",
            "repetitions": 1,
            "sweeps": [
                {
                    "parameter": "maximum_vehicles",
                    "values": [10.5]
                }
            ]
        }
    )json";

    EXPECT_THROW(static_cast<void>(ExperimentPlanLoader::loadFromJson(incorrectMaximumVehicles)),
                 std::invalid_argument);
}

} // namespace