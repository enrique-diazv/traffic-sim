#include "trafficsim/experiments/BatchExperimentConfig.h"

#include <gtest/gtest.h>

#include <limits>

namespace
{

using trafficsim::BatchExperimentConfig;
using trafficsim::ExperimentVariant;
using trafficsim::SimulationConfig;

BatchExperimentConfig validExperimentConfig()
{
    return {
        .repetitions = 3U,
        .seedStride = 10U,
        .variants =
            {
                ExperimentVariant{
                    .name = "baseline",
                    .simulationConfig = SimulationConfig{},
                },
                ExperimentVariant{
                    .name = "alternative",
                    .simulationConfig = SimulationConfig{},
                },
            },
    };
}

TEST(BatchExperimentConfigTests, AcceptsValidConfiguration)
{
    const auto config = validExperimentConfig();

    EXPECT_NO_THROW(config.validate());
}

TEST(BatchExperimentConfigTests, RejectsInvalidTopLevelConfiguration)
{
    auto config = validExperimentConfig();

    config.repetitions = 0U;
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config = validExperimentConfig();
    config.seedStride = 0U;
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config = validExperimentConfig();
    config.variants.clear();
    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(BatchExperimentConfigTests, RejectsMissingOrDuplicateVariantNames)
{
    auto config = validExperimentConfig();

    config.variants.front().name.clear();
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config = validExperimentConfig();
    config.variants.back().name = config.variants.front().name;
    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(BatchExperimentConfigTests, RejectsInvalidSimulationConfiguration)
{
    auto config = validExperimentConfig();
    config.variants.front().simulationConfig.timeStepSeconds = 0.0;

    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(BatchExperimentConfigTests, RejectsSeedSequenceOverflow)
{
    auto config = validExperimentConfig();
    config.repetitions = 2U;
    config.seedStride = 1U;
    config.variants.front().simulationConfig.randomSeed = std::numeric_limits<std::uint64_t>::max();

    EXPECT_THROW(config.validate(), std::invalid_argument);
}

} // namespace