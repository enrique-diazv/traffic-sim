#include "trafficsim/core/RandomGenerator.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::RandomGenerator;

TEST(RandomGeneratorTests, SameSeedProducesSameSequence)
{
    RandomGenerator first{42};
    RandomGenerator second{42};

    for (int sample = 0; sample < 100; ++sample)
    {
        EXPECT_EQ(first.randomInt(-50, 50), second.randomInt(-50, 50));
        EXPECT_DOUBLE_EQ(first.randomDouble(-10.0, 10.0), second.randomDouble(-10.0, 10.0));
    }
}

TEST(RandomGeneratorTests, ProducesIntegersInsideInclusiveBounds)
{
    RandomGenerator generator{123};

    for (int sample = 0; sample < 100; ++sample)
    {
        const int value = generator.randomInt(-5, 5);

        EXPECT_GE(value, -5);
        EXPECT_LE(value, 5);
    }
}

TEST(RandomGeneratorTests, ProducesDoublesInsideBounds)
{
    RandomGenerator generator{456};

    for (int sample = 0; sample < 100; ++sample)
    {
        const double value = generator.randomDouble(-2.5, 3.5);

        EXPECT_GE(value, -2.5);
        EXPECT_LT(value, 3.5);
    }
}

TEST(RandomGeneratorTests, SupportsEqualBounds)
{
    RandomGenerator generator{789};

    EXPECT_EQ(generator.randomInt(7, 7), 7);
    EXPECT_DOUBLE_EQ(generator.randomDouble(2.5, 2.5), 2.5);
}

TEST(RandomGeneratorTests, RejectsInvalidBounds)
{
    RandomGenerator generator{101};

    EXPECT_THROW(static_cast<void>(generator.randomInt(5, -5)), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(generator.randomDouble(5.0, -5.0)), std::invalid_argument);
    EXPECT_THROW(
        static_cast<void>(generator.randomDouble(0.0, std::numeric_limits<double>::infinity())),
        std::invalid_argument);
    EXPECT_THROW(
        static_cast<void>(generator.randomDouble(std::numeric_limits<double>::quiet_NaN(), 1.0)),
        std::invalid_argument);
}
} // namespace
