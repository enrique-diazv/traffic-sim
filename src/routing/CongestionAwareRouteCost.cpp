#include "trafficsim/routing/CongestionAwareRouteCost.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trafficsim
{

namespace
{

double penaltyMultiplier(const CongestionCostConfig &config, CongestionState state)
{
    switch (state)
    {
    case CongestionState::FreeFlow:
        return 1.0;
    case CongestionState::Moderate:
        return config.moderatePenaltyMultiplier;
    case CongestionState::Congested:
        return config.congestedPenaltyMultiplier;
    case CongestionState::Gridlock:
        return config.gridlockPenaltyMultiplier;
    }

    throw std::invalid_argument{"Unknown congestion state"};
}

} // namespace

void CongestionCostConfig::validate() const
{
    if (!std::isfinite(minimumSpeedRatio) || minimumSpeedRatio <= 0.0 || minimumSpeedRatio > 1.0)
    {
        throw std::invalid_argument{
            "Minimum speed ratio must be greater than zero and at most one"};
    }

    if (!std::isfinite(moderatePenaltyMultiplier) || !std::isfinite(congestedPenaltyMultiplier) ||
        !std::isfinite(gridlockPenaltyMultiplier))
    {
        throw std::invalid_argument{"Congestion penalties must be finite"};
    }

    if (moderatePenaltyMultiplier < 1.0 || congestedPenaltyMultiplier < moderatePenaltyMultiplier ||
        gridlockPenaltyMultiplier < congestedPenaltyMultiplier)
    {
        throw std::invalid_argument{"Congestion penalties must be ordered and not less than one"};
    }
}

CongestionAwareRouteCost::CongestionAwareRouteCost(CongestionCostConfig config) : config_{config}
{
    config_.validate();
}

double CongestionAwareRouteCost::calculate(const Road &road) const
{
    const auto cost = road.lengthMeters() / road.speedLimitMetersPerSecond();

    if (!std::isfinite(cost))
    {
        throw std::overflow_error{"Free-flow road cost overflow"};
    }

    return cost;
}

double CongestionAwareRouteCost::calculate(const Road &road,
                                           const RoadTrafficMetrics &metrics) const
{
    if (metrics.roadId != road.id())
    {
        throw std::invalid_argument{"Road metrics identifier does not match road"};
    }

    if (!std::isfinite(metrics.speedRatio) || metrics.speedRatio < 0.0 ||
        !std::isfinite(metrics.occupancy) || metrics.occupancy < 0.0)
    {
        throw std::invalid_argument{"Road congestion metrics must be finite and non-negative"};
    }

    if (metrics.vehicleCount == 0)
    {
        return calculate(road);
    }

    const auto effectiveSpeedRatio = std::clamp(metrics.speedRatio, config_.minimumSpeedRatio, 1.0);
    const auto cost =
        calculate(road) / effectiveSpeedRatio * penaltyMultiplier(config_, metrics.congestionState);

    if (!std::isfinite(cost))
    {
        throw std::overflow_error{"Congestion-aware road cost overflow"};
    }

    return cost;
}

const CongestionCostConfig &CongestionAwareRouteCost::config() const noexcept
{
    return config_;
}

} // namespace trafficsim