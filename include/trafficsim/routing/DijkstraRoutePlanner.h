#ifndef TRAFFICSIM_ROUTING_DIJKSTRA_ROUTE_PLANNER_H
#define TRAFFICSIM_ROUTING_DIJKSTRA_ROUTE_PLANNER_H

#include "trafficsim/routing/RoutePlanner.h"

namespace trafficsim
{

class DijkstraRoutePlanner final : public RoutePlanner
{
  public:
    [[nodiscard]] std::optional<Route> calculateRoute(const RoadNetwork &network,
                                                      IntersectionId start,
                                                      IntersectionId destination) const override;
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_DIJKSTRA_ROUTE_PLANNER_H