#include "trafficsim/experiments/ParameterSweep.h"

#include <gtest/gtest.h>

#include <span>
#include <vector>

namespace
{

using trafficsim::ParameterSweep;
using trafficsim::ParameterSweepGenerator;
using trafficsim::SimulationConfig;
using trafficsim::SweepParameter;

TEST(ParameterSweepTests, ReturnsBaselineWhenNoSweepsAreProvided)
{
    const SimulationConfig baseConfig;

    const auto variants =
        ParameterSweepGenerator::generate(baseConfig, std::span<const ParameterSweep>{});

    ASSERT_EQ(variants.size(), 1U);
    EXPECT_EQ(variants.front().name, "baseline");
    EXPECT_DOUBLE_EQ(variants.front().simulationConfig.timeStepSeconds, baseConfig.timeStepSeconds);
    EXPECT_EQ(variants.front().simulationConfig.maximumVehicles, baseConfig.maximumVehicles);
}

TEST(ParameterSweepTests, GeneratesCartesianProductInDeterministicOrder)
{
    const SimulationConfig baseConfig;
    const std::vector<ParameterSweep> sweeps{
        ParameterSweep{
            .parameter = SweepParameter::TimeStepSeconds,
            .values = {0.1, 0.2},
        },
        ParameterSweep{
            .parameter = SweepParameter::MaximumVehicles,
            .values = {std::size_t{10U}, std::size_t{20U}, std::size_t{30U}},
        },
    };

    const auto variants = ParameterSweepGenerator::generate(baseConfig, sweeps);

    ASSERT_EQ(variants.size(), 6U);

    EXPECT_EQ(variants[0].name, "time_step_seconds=0.1;maximum_vehicles=10");
    EXPECT_EQ(variants[1].name, "time_step_seconds=0.1;maximum_vehicles=20");
    EXPECT_EQ(variants[2].name, "time_step_seconds=0.1;maximum_vehicles=30");
    EXPECT_EQ(variants[3].name, "time_step_seconds=0.2;maximum_vehicles=10");
    EXPECT_EQ(variants[4].name, "time_step_seconds=0.2;maximum_vehicles=20");
    EXPECT_EQ(variants[5].name, "time_step_seconds=0.2;maximum_vehicles=30");

    EXPECT_DOUBLE_EQ(variants[0].simulationConfig.timeStepSeconds, 0.1);
    EXPECT_EQ(variants[0].simulationConfig.maximumVehicles, 10U);
    EXPECT_DOUBLE_EQ(variants[5].simulationConfig.timeStepSeconds, 0.2);
    EXPECT_EQ(variants[5].simulationConfig.maximumVehicles, 30U);
}

TEST(ParameterSweepTests, RejectsEmptyDuplicateAndIncorrectlyTypedSweeps)
{
    const SimulationConfig baseConfig;

    const std::vector<ParameterSweep> emptyValues{
        ParameterSweep{
            .parameter = SweepParameter::TimeStepSeconds,
            .values = {},
        },
    };
    EXPECT_THROW(static_cast<void>(ParameterSweepGenerator::generate(baseConfig, emptyValues)),
                 std::invalid_argument);

    const std::vector<ParameterSweep> duplicateParameters{
        ParameterSweep{
            .parameter = SweepParameter::TimeStepSeconds,
            .values = {0.1},
        },
        ParameterSweep{
            .parameter = SweepParameter::TimeStepSeconds,
            .values = {0.2},
        },
    };
    EXPECT_THROW(
        static_cast<void>(ParameterSweepGenerator::generate(baseConfig, duplicateParameters)),
        std::invalid_argument);

    const std::vector<ParameterSweep> incorrectType{
        ParameterSweep{
            .parameter = SweepParameter::TimeStepSeconds,
            .values = {std::size_t{1U}},
        },
    };
    EXPECT_THROW(static_cast<void>(ParameterSweepGenerator::generate(baseConfig, incorrectType)),
                 std::invalid_argument);
}

TEST(ParameterSweepTests, RejectsGeneratedInvalidSimulationConfiguration)
{
    const SimulationConfig baseConfig;
    const std::vector<ParameterSweep> sweeps{
        ParameterSweep{
            .parameter = SweepParameter::TimeStepSeconds,
            .values = {0.0},
        },
    };

    EXPECT_THROW(static_cast<void>(ParameterSweepGenerator::generate(baseConfig, sweeps)),
                 std::invalid_argument);
}

} // namespace