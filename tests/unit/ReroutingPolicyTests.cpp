#include "trafficsim/routing/ReroutingPolicy.h"

#include <gtest/gtest.h>

#include <limits>
#include <optional>
#include <stdexcept>

namespace
{

using trafficsim::CongestionState;
using trafficsim::ReroutingConfig;
using trafficsim::ReroutingPolicy;
using trafficsim::RouteComparisonResult;

RouteComparisonResult betterRoute(double relativeImprovement)
{
    return RouteComparisonResult{
        .currentCost = 100.0,
        .candidateCost = 100.0 * (1.0 - relativeImprovement),
        .relativeImprovement = relativeImprovement,
        .candidateIsBetter = true,
    };
}

TEST(ReroutingPolicyTests, EvaluatesOnlyWhenIntervalIsDue)
{
    const ReroutingPolicy policy;

    EXPECT_TRUE(policy.evaluationDue(0.0, std::nullopt));
    EXPECT_FALSE(policy.evaluationDue(4.9, 0.0));
    EXPECT_TRUE(policy.evaluationDue(5.0, 0.0));
    EXPECT_TRUE(policy.evaluationDue(10.0, 5.0));
}

TEST(ReroutingPolicyTests, ReroutesForSevereCongestion)
{
    const ReroutingPolicy policy;

    EXPECT_TRUE(policy.shouldReroute(CongestionState::Congested, betterRoute(0.05)));
    EXPECT_TRUE(policy.shouldReroute(CongestionState::Gridlock, betterRoute(0.01)));
}

TEST(ReroutingPolicyTests, ReroutesForSubstantialImprovement)
{
    const ReroutingPolicy policy;

    EXPECT_TRUE(policy.shouldReroute(CongestionState::Moderate, betterRoute(0.20)));
    EXPECT_FALSE(policy.shouldReroute(CongestionState::Moderate, betterRoute(0.10)));
}

TEST(ReroutingPolicyTests, NeverSelectsRouteThatIsNotBetter)
{
    const ReroutingPolicy policy;
    const RouteComparisonResult worseRoute{
        .currentCost = 100.0,
        .candidateCost = 120.0,
        .relativeImprovement = -0.20,
        .candidateIsBetter = false,
    };

    EXPECT_FALSE(policy.shouldReroute(CongestionState::Gridlock, worseRoute));
}

TEST(ReroutingPolicyTests, RejectsInvalidConfigurationAndTimes)
{
    auto config = ReroutingConfig{};
    config.evaluationIntervalSeconds = 0.0;
    EXPECT_THROW(static_cast<void>(ReroutingPolicy{config}), std::invalid_argument);

    config = ReroutingConfig{};
    config.minimumImprovementRatio = 1.1;
    EXPECT_THROW(static_cast<void>(ReroutingPolicy{config}), std::invalid_argument);

    config = ReroutingConfig{};
    config.severeCongestionThreshold = CongestionState::Moderate;
    EXPECT_THROW(static_cast<void>(ReroutingPolicy{config}), std::invalid_argument);

    const ReroutingPolicy policy;

    EXPECT_THROW(static_cast<void>(policy.evaluationDue(-1.0, std::nullopt)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(
                     policy.evaluationDue(std::numeric_limits<double>::infinity(), std::nullopt)),
                 std::invalid_argument);
    EXPECT_THROW(static_cast<void>(policy.evaluationDue(5.0, 6.0)), std::invalid_argument);
}

} // namespace