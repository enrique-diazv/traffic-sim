#include "trafficsim/core/SimulationClock.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::SimulationClock;

TEST(SimulationClockTests, StartsAtZero)
{
    const SimulationClock clock{0.1};

    EXPECT_EQ(clock.tickCount(), 0U);
    EXPECT_DOUBLE_EQ(clock.currentTimeSeconds(), 0.0);
    EXPECT_DOUBLE_EQ(clock.timeStepSeconds(), 0.1);
}

TEST(SimulationClockTests, AdvancesByFixedTicks)
{
    SimulationClock clock{0.1};

    clock.advance();
    clock.advance();
    clock.advance();

    EXPECT_EQ(clock.tickCount(), 3U);
    EXPECT_NEAR(clock.currentTimeSeconds(), 0.3, 1.0e-12);
}

TEST(SimulationClockTests, ResetsToZero)
{
    SimulationClock clock{0.5};
    clock.advance();
    clock.advance();

    clock.reset();

    EXPECT_EQ(clock.tickCount(), 0U);
    EXPECT_DOUBLE_EQ(clock.currentTimeSeconds(), 0.0);
}

TEST(SimulationClockTests, RejectsInvalidTimeStep)
{
    EXPECT_THROW((SimulationClock{0.0}), std::invalid_argument);
    EXPECT_THROW((SimulationClock{-1.0}), std::invalid_argument);
    EXPECT_THROW(
        (SimulationClock{std::numeric_limits<double>::infinity()}),
        std::invalid_argument
    );
    EXPECT_THROW(
        (SimulationClock{std::numeric_limits<double>::quiet_NaN()}),
        std::invalid_argument
    );
}

} // namespace
