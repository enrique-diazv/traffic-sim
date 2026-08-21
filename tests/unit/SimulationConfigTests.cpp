#include "trafficsim/core/SimulationConfig.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::SimulationConfig;

TEST(SimulationConfigTests, DefaultConfigurationIsValid)
{
    const SimulationConfig config;

    EXPECT_NO_THROW(config.validate());
}

TEST(SimulationConfigTests, RejectsInvalidDuration)
{
    SimulationConfig config;

    config.durationSeconds = 0.0;
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config.durationSeconds = -1.0;
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config.durationSeconds = std::numeric_limits<double>::infinity();
    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(SimulationConfigTests, RejectsInvalidTimeStep)
{
    SimulationConfig config;

    config.timeStepSeconds = 0.0;
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config.timeStepSeconds = config.durationSeconds + 1.0;
    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(SimulationConfigTests, RejectsZeroVehicleLimit)
{
    SimulationConfig config;
    config.maximumVehicles = 0;

    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(SimulationConfigTests, RejectsInvalidDefaultDynamics)
{
    SimulationConfig config;
    config.defaultVehicleDynamics.maximumSpeedMetersPerSecond = 0.0;

    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(SimulationConfigTests, RejectsInvalidFollowingDistance)
{
    SimulationConfig config;
    config.minimumFollowingDistanceMeters = -1.0;

    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(SimulationConfigTests, RejectsInvalidReactionTime)
{
    SimulationConfig config;

    config.reactionTimeSeconds = 0.0;
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config.reactionTimeSeconds = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(SimulationConfigTests, RejectsInvalidRoutingConfiguration)
{
    SimulationConfig config;

    config.rerouting.evaluationIntervalSeconds = 0.0;
    EXPECT_THROW(config.validate(), std::invalid_argument);

    config = SimulationConfig{};
    config.congestionCost.minimumSpeedRatio = 0.0;
    EXPECT_THROW(config.validate(), std::invalid_argument);
}

} // namespace
