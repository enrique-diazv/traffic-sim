#include "trafficsim/routing/Route.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <vector>

namespace
{

using trafficsim::RoadId;
using trafficsim::Route;

TEST(RouteTests, StoresSegmentsAndDistance)
{
    const std::vector<RoadId> roadIds{10, 20, 30};
    const Route route{roadIds, 450.0};

    ASSERT_EQ(route.segmentCount(), 3U);
    EXPECT_EQ(route.segments()[0], 10U);
    EXPECT_EQ(route.segments()[1], 20U);
    EXPECT_EQ(route.segments()[2], 30U);
    EXPECT_DOUBLE_EQ(route.totalDistanceMeters(), 450.0);
}

TEST(RouteTests, ExposesOnlyRemainingSegments)
{
    Route route{{10, 20, 30}, 450.0};

    ASSERT_EQ(route.remainingSegments().size(), 3U);
    EXPECT_EQ(route.remainingSegments()[0], 10U);

    ASSERT_TRUE(route.advance());

    ASSERT_EQ(route.remainingSegments().size(), 2U);
    EXPECT_EQ(route.remainingSegments()[0], 20U);
    EXPECT_EQ(route.remainingSegments()[1], 30U);

    ASSERT_TRUE(route.advance());
    ASSERT_TRUE(route.advance());

    EXPECT_TRUE(route.remainingSegments().empty());
    EXPECT_EQ(route.segments().size(), 3U);
}

TEST(RouteTests, ReplacesOnlyCurrentAndFutureSegments)
{
    Route route{{10, 20, 30}, 450.0};

    ASSERT_TRUE(route.advance());

    route.replaceRemainingSegments({20, 40, 50}, 600.0);

    ASSERT_EQ(route.segments().size(), 4U);
    EXPECT_EQ(route.segments()[0], 10U);
    EXPECT_EQ(route.segments()[1], 20U);
    EXPECT_EQ(route.segments()[2], 40U);
    EXPECT_EQ(route.segments()[3], 50U);

    ASSERT_EQ(route.remainingSegments().size(), 3U);
    EXPECT_EQ(route.currentRoad(), 20U);
    EXPECT_DOUBLE_EQ(route.totalDistanceMeters(), 600.0);
}

TEST(RouteTests, RejectsInvalidRemainingSegmentReplacement)
{
    Route route{{10, 20}, 200.0};

    EXPECT_THROW(route.replaceRemainingSegments({}, 100.0), std::invalid_argument);
    EXPECT_THROW(route.replaceRemainingSegments({99}, 100.0), std::invalid_argument);
    EXPECT_THROW(route.replaceRemainingSegments({10}, -1.0), std::invalid_argument);

    ASSERT_TRUE(route.advance());
    ASSERT_TRUE(route.advance());

    EXPECT_THROW(route.replaceRemainingSegments({20}, 200.0), std::logic_error);
}

TEST(RouteTests, TracksCurrentAndNextSegments)
{
    Route route{{10, 20, 30}, 450.0};

    ASSERT_TRUE(route.currentRoad().has_value());
    EXPECT_EQ(*route.currentRoad(), 10U);
    ASSERT_TRUE(route.nextRoad().has_value());
    EXPECT_EQ(*route.nextRoad(), 20U);

    EXPECT_TRUE(route.advance());
    EXPECT_EQ(*route.currentRoad(), 20U);
    EXPECT_EQ(*route.nextRoad(), 30U);

    EXPECT_TRUE(route.advance());
    EXPECT_EQ(*route.currentRoad(), 30U);
    EXPECT_FALSE(route.nextRoad().has_value());

    EXPECT_TRUE(route.advance());
    EXPECT_TRUE(route.isComplete());
    EXPECT_FALSE(route.currentRoad().has_value());
    EXPECT_FALSE(route.advance());
}

TEST(RouteTests, EmptyRouteIsComplete)
{
    Route route{std::vector<RoadId>{}, 0.0};

    EXPECT_TRUE(route.isComplete());
    EXPECT_EQ(route.segmentCount(), 0U);
    EXPECT_FALSE(route.currentRoad().has_value());
    EXPECT_FALSE(route.nextRoad().has_value());
    EXPECT_FALSE(route.advance());
}

TEST(RouteTests, RejectsInvalidDistance)
{
    const std::vector<RoadId> roadIds{10};

    EXPECT_THROW((Route{roadIds, -1.0}), std::invalid_argument);
    EXPECT_THROW((Route{roadIds, std::numeric_limits<double>::infinity()}), std::invalid_argument);
}

TEST(RouteTests, RejectsDistanceForEmptyRoute)
{
    const std::vector<RoadId> roadIds;

    EXPECT_THROW((Route{roadIds, 100.0}), std::invalid_argument);
}

TEST(RouteTests, RejectsZeroDistanceForNonEmptyRoute)
{
    const std::vector<RoadId> roadIds{10};

    EXPECT_THROW((Route{roadIds, 0.0}), std::invalid_argument);
}

} // namespace