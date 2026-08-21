#ifndef TRAFFICSIM_ROUTING_ASTAR_ROUTE_PLANNER_H
#define TRAFFICSIM_ROUTING_ASTAR_ROUTE_PLANNER_H

#include "trafficsim/routing/CongestionAwareRouteCost.h"
#include "trafficsim/routing/RoutePlanner.h"

#include <span>

namespace trafficsim
{

class AStarRoutePlanner final : public RoutePlanner
{
  public:
    explicit AStarRoutePlanner(CongestionAwareRouteCost routeCost = CongestionAwareRouteCost{});

    [[nodiscard]] std::optional<Route> calculateRoute(const RoadNetwork &network,
                                                      IntersectionId start,
                                                      IntersectionId destination) const override;

    [[nodiscard]] std::optional<Route>
    calculateRoute(const RoadNetwork &network, IntersectionId start, IntersectionId destination,
                   std::span<const RoadTrafficMetrics> roadMetrics) const;

  private:
    CongestionAwareRouteCost routeCost_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_ASTAR_ROUTE_PLANNER_H