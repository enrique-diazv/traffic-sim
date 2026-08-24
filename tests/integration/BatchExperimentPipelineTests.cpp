#include "trafficsim/experiments/BatchExperimentRunner.h"
#include "trafficsim/experiments/ExperimentAggregator.h"
#include "trafficsim/experiments/ExperimentPlanLoader.h"
#include "trafficsim/experiments/ParameterSweep.h"
#include "trafficsim/io/ScenarioLoader.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{

TEST(BatchExperimentPipelineTests, RunsConfiguredExperimentPlan)
{
    const auto plan = trafficsim::ExperimentPlanLoader::loadFromFile(
        std::filesystem::path{"experiments"} / "basic_sweep.json");

    const auto scenario = trafficsim::ScenarioLoader::loadFromFile(plan.scenarioPath);

    const trafficsim::BatchExperimentConfig batchConfig{
        .repetitions = plan.repetitions,
        .seedStride = plan.seedStride,
        .variants = trafficsim::ParameterSweepGenerator::generate(scenario.config, plan.sweeps),
    };

    const auto runResults = trafficsim::BatchExperimentRunner::run(scenario, batchConfig);
    const auto aggregatedResults = trafficsim::ExperimentAggregator::aggregate(runResults);

    ASSERT_EQ(batchConfig.variants.size(), 6U);
    ASSERT_EQ(runResults.size(), 18U);
    ASSERT_EQ(aggregatedResults.size(), 6U);

    EXPECT_EQ(runResults[0].randomSeed, 42U);
    EXPECT_EQ(runResults[1].randomSeed, 142U);
    EXPECT_EQ(runResults[2].randomSeed, 242U);

    for (const auto &result : aggregatedResults)
    {
        EXPECT_EQ(result.runCount, 3U);
    }
}

} // namespace