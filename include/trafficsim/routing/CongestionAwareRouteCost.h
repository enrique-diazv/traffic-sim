#ifndef TRAFFICSIM_ROUTING_CONGESTION_AWARE_ROUTE_COST_H
#define TRAFFICSIM_ROUTING_CONGESTION_AWARE_ROUTE_COST_H

#include "trafficsim/network/Road.h"
#include "trafficsim/traffic/RoadTrafficMetrics.h"

namespace trafficsim
{

struct CongestionCostConfig
{
    double minimumSpeedRatio{0.1};
    double moderatePenaltyMultiplier{1.1};
    double congestedPenaltyMultiplier{1.5};
    double gridlockPenaltyMultiplier{3.0};

    void validate() const;
};

class CongestionAwareRouteCost final
{
  public:
    explicit CongestionAwareRouteCost(CongestionCostConfig config = {});

    [[nodiscard]] double calculate(const Road &road) const;
    [[nodiscard]] double calculate(const Road &road, const RoadTrafficMetrics &metrics) const;

    [[nodiscard]] const CongestionCostConfig &config() const noexcept;

  private:
    CongestionCostConfig config_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_CONGESTION_AWARE_ROUTE_COST_H