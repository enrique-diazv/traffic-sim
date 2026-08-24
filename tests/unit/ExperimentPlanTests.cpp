#include "trafficsim/experiments/ExperimentPlan.h"

#include <gtest/gtest.h>

namespace
{

using trafficsim::ExperimentPlan;
using trafficsim::ParameterSweep;
using trafficsim::SweepParameter;

ExperimentPlan validPlan()
{
    return {
        .scenarioPath = "scenarios/basic.json",
        .outputDirectory = "results/experiments",
        .repetitions = 3U,
        .seedStride = 100U,
        .sweeps =
            {
                ParameterSweep{
                    .parameter = SweepParameter::TimeStepSeconds,
                    .values = {0.1, 0.2},
                },
            },
    };
}

TEST(ExperimentPlanTests, AcceptsValidPlanAndBaselineOnlyPlan)
{
    const auto plan = validPlan();
    EXPECT_NO_THROW(plan.validate());

    auto baselinePlan = validPlan();
    baselinePlan.sweeps.clear();

    EXPECT_NO_THROW(baselinePlan.validate());
}

TEST(ExperimentPlanTests, RejectsInvalidPathsAndRunConfiguration)
{
    auto plan = validPlan();

    plan.scenarioPath.clear();
    EXPECT_THROW(plan.validate(), std::invalid_argument);

    plan = validPlan();
    plan.outputDirectory.clear();
    EXPECT_THROW(plan.validate(), std::invalid_argument);

    plan = validPlan();
    plan.repetitions = 0U;
    EXPECT_THROW(plan.validate(), std::invalid_argument);

    plan = validPlan();
    plan.seedStride = 0U;
    EXPECT_THROW(plan.validate(), std::invalid_argument);
}

TEST(ExperimentPlanTests, RejectsEmptyOrDuplicateSweeps)
{
    auto plan = validPlan();

    plan.sweeps.front().values.clear();
    EXPECT_THROW(plan.validate(), std::invalid_argument);

    plan = validPlan();
    plan.sweeps.push_back(plan.sweeps.front());
    EXPECT_THROW(plan.validate(), std::invalid_argument);
}

} // namespace