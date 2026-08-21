#include "trafficsim/routing/ReroutingPolicy.h"

#include <cmath>
#include <stdexcept>

namespace trafficsim
{

void ReroutingConfig::validate() const
{
    if (!std::isfinite(evaluationIntervalSeconds) || evaluationIntervalSeconds <= 0.0)
    {
        throw std::invalid_argument{"Rerouting evaluation interval must be finite and positive"};
    }

    if (!std::isfinite(minimumImprovementRatio) || minimumImprovementRatio < 0.0 ||
        minimumImprovementRatio > 1.0)
    {
        throw std::invalid_argument{"Minimum rerouting improvement must be between zero and one"};
    }

    if (severeCongestionThreshold != CongestionState::Congested &&
        severeCongestionThreshold != CongestionState::Gridlock)
    {
        throw std::invalid_argument{"Severe congestion threshold must be congested or gridlock"};
    }
}

ReroutingPolicy::ReroutingPolicy(ReroutingConfig config) : config_{config}
{
    config_.validate();
}

bool ReroutingPolicy::evaluationDue(double simulationTimeSeconds,
                                    std::optional<double> lastEvaluationTimeSeconds) const
{
    if (!std::isfinite(simulationTimeSeconds) || simulationTimeSeconds < 0.0)
    {
        throw std::invalid_argument{"Rerouting simulation time must be finite and non-negative"};
    }

    if (!lastEvaluationTimeSeconds.has_value())
    {
        return true;
    }

    if (!std::isfinite(*lastEvaluationTimeSeconds) || *lastEvaluationTimeSeconds < 0.0 ||
        *lastEvaluationTimeSeconds > simulationTimeSeconds)
    {
        throw std::invalid_argument{"Last rerouting evaluation time is invalid"};
    }

    return simulationTimeSeconds - *lastEvaluationTimeSeconds >= config_.evaluationIntervalSeconds;
}

bool ReroutingPolicy::shouldReroute(CongestionState worstCurrentRouteState,
                                    const RouteComparisonResult &comparison) const noexcept
{
    if (!comparison.candidateIsBetter)
    {
        return false;
    }

    const auto hasSevereCongestion = worstCurrentRouteState >= config_.severeCongestionThreshold;
    const auto hasSubstantialImprovement =
        comparison.relativeImprovement >= config_.minimumImprovementRatio;

    return hasSevereCongestion || hasSubstantialImprovement;
}

const ReroutingConfig &ReroutingPolicy::config() const noexcept
{
    return config_;
}

} // namespace trafficsim