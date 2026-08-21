#ifndef TRAFFICSIM_ROUTING_ROUTE_COMPARATOR_H
#define TRAFFICSIM_ROUTING_ROUTE_COMPARATOR_H

#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/routing/CongestionAwareRouteCost.h"

#include <span>

namespace trafficsim
{

struct RouteComparisonResult
{
    double currentCost{};
    double candidateCost{};
    double relativeImprovement{};
    bool candidateIsBetter{};
};

class RouteComparator final
{
  public:
    explicit RouteComparator(CongestionAwareRouteCost routeCost = CongestionAwareRouteCost{});

    [[nodiscard]] double calculateCost(const RoadNetwork &network, std::span<const RoadId> roadIds,
                                       std::span<const RoadTrafficMetrics> roadMetrics = {}) const;

    [[nodiscard]] RouteComparisonResult
    compare(const RoadNetwork &network, std::span<const RoadId> currentRoadIds,
            std::span<const RoadId> candidateRoadIds,
            std::span<const RoadTrafficMetrics> roadMetrics = {}) const;

  private:
    CongestionAwareRouteCost routeCost_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_ROUTE_COMPARATOR_H