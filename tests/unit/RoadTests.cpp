#include "trafficsim/network/Road.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::Road;
using trafficsim::RoadId;
using trafficsim::RoadProperties;

constexpr RoadProperties validRoadProperties()
{
    return RoadProperties{
        .origin = 1,
        .destination = 2,
        .lengthMeters = 250.0,
        .speedLimitMetersPerSecond = 13.9,
        .laneCount = 2,
        .capacity = 40,
    };
}

TEST(RoadTests, StoresIdentifierAndProperties)
{
    constexpr RoadId roadId{10};
    constexpr auto properties = validRoadProperties();

    const Road road{roadId, properties};

    EXPECT_EQ(road.id(), roadId);
    EXPECT_EQ(road.origin(), properties.origin);
    EXPECT_EQ(road.destination(), properties.destination);
    EXPECT_DOUBLE_EQ(road.lengthMeters(), properties.lengthMeters);
    EXPECT_DOUBLE_EQ(road.speedLimitMetersPerSecond(), properties.speedLimitMetersPerSecond);
    EXPECT_EQ(road.laneCount(), properties.laneCount);
    EXPECT_EQ(road.capacity(), properties.capacity);
}

TEST(RoadTests, RejectsEqualOriginAndDestination)
{
    auto properties = validRoadProperties();
    properties.destination = properties.origin;

    EXPECT_THROW((Road{1, properties}), std::invalid_argument);
}

TEST(RoadTests, RejectsInvalidLength)
{
    auto properties = validRoadProperties();

    properties.lengthMeters = 0.0;
    EXPECT_THROW((Road{1, properties}), std::invalid_argument);

    properties.lengthMeters = -1.0;
    EXPECT_THROW((Road{1, properties}), std::invalid_argument);

    properties.lengthMeters = std::numeric_limits<double>::infinity();
    EXPECT_THROW((Road{1, properties}), std::invalid_argument);
}

TEST(RoadTests, RejectsInvalidSpeedLimit)
{
    auto properties = validRoadProperties();

    properties.speedLimitMetersPerSecond = 0.0;
    EXPECT_THROW((Road{1, properties}), std::invalid_argument);

    properties.speedLimitMetersPerSecond = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW((Road{1, properties}), std::invalid_argument);
}

TEST(RoadTests, RejectsZeroLanes)
{
    auto properties = validRoadProperties();
    properties.laneCount = 0;

    EXPECT_THROW((Road{1, properties}), std::invalid_argument);
}

TEST(RoadTests, RejectsZeroCapacity)
{
    auto properties = validRoadProperties();
    properties.capacity = 0;

    EXPECT_THROW((Road{1, properties}), std::invalid_argument);
}

} // namespace