#include "trafficsim/network/Intersection.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::Intersection;
using trafficsim::IntersectionId;
using trafficsim::Position;

TEST(IntersectionTests, StoresIdentifierAndPosition)
{
    constexpr IntersectionId intersectionId{7};
    constexpr Position position{12.5, -4.25};

    const Intersection intersection{intersectionId, position};

    EXPECT_EQ(intersection.id(), intersectionId);
    EXPECT_EQ(intersection.position(), position);
}

TEST(IntersectionTests, RejectsInfinitePosition)
{
    const auto infinity = std::numeric_limits<double>::infinity();

    EXPECT_THROW((Intersection{1, Position{infinity, 0.0}}), std::invalid_argument);
}

TEST(IntersectionTests, RejectsNotANumberPosition)
{
    const auto notANumber = std::numeric_limits<double>::quiet_NaN();

    EXPECT_THROW((Intersection{1, Position{0.0, notANumber}}), std::invalid_argument);
}

} // namespace