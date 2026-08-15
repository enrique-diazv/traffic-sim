#ifndef TRAFFICSIM_ROUTING_ROUTE_PLANNER_H
#define TRAFFICSIM_ROUTING_ROUTE_PLANNER_H

#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/routing/Route.h"

#include <optional>

namespace trafficsim
{

class RoutePlanner
{
  public:
    virtual ~RoutePlanner() = default;

    [[nodiscard]] virtual std::optional<Route> calculateRoute(const RoadNetwork &network,
                                                              IntersectionId start,
                                                              IntersectionId destination) const = 0;
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_ROUTE_PLANNER_H