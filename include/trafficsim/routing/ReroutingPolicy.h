#ifndef TRAFFICSIM_ROUTING_REROUTING_POLICY_H
#define TRAFFICSIM_ROUTING_REROUTING_POLICY_H

#include "trafficsim/routing/RouteComparator.h"

#include <optional>

namespace trafficsim
{

struct ReroutingConfig
{
    double evaluationIntervalSeconds{5.0};
    double minimumImprovementRatio{0.15};
    CongestionState severeCongestionThreshold{CongestionState::Congested};

    void validate() const;
};

class ReroutingPolicy final
{
  public:
    explicit ReroutingPolicy(ReroutingConfig config = {});

    [[nodiscard]] bool evaluationDue(double simulationTimeSeconds,
                                     std::optional<double> lastEvaluationTimeSeconds) const;

    [[nodiscard]] bool shouldReroute(CongestionState worstCurrentRouteState,
                                     const RouteComparisonResult &comparison) const noexcept;

    [[nodiscard]] const ReroutingConfig &config() const noexcept;

  private:
    ReroutingConfig config_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_REROUTING_POLICY_H