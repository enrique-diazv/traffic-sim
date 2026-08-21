#ifndef TRAFFICSIM_ROUTING_DYNAMIC_ROUTING_MANAGER_H
#define TRAFFICSIM_ROUTING_DYNAMIC_ROUTING_MANAGER_H

#include "trafficsim/routing/AStarRoutePlanner.h"
#include "trafficsim/routing/ReroutingPolicy.h"
#include "trafficsim/traffic/RoadTrafficMonitor.h"
#include "trafficsim/vehicles/VehicleManager.h"

#include <cstddef>
#include <unordered_map>

namespace trafficsim
{

struct DynamicRoutingResult
{
    std::size_t evaluatedVehicles{};
    std::size_t reroutedVehicles{};
};

class DynamicRoutingManager final
{
  public:
    explicit DynamicRoutingManager(ReroutingConfig reroutingConfig = {},
                                   CongestionCostConfig congestionCostConfig = {});

    [[nodiscard]] DynamicRoutingResult update(double simulationTimeSeconds,
                                              const RoadNetwork &network,
                                              const RoadTrafficMonitor &trafficMonitor,
                                              VehicleManager &vehicleManager);

    void reset() noexcept;

    [[nodiscard]] std::size_t totalEvaluations() const noexcept;
    [[nodiscard]] std::size_t totalReroutes() const noexcept;

  private:
    AStarRoutePlanner routePlanner_;
    RouteComparator routeComparator_;
    ReroutingPolicy reroutingPolicy_;
    std::unordered_map<VehicleId, double> lastEvaluationTimeByVehicle_;
    std::size_t totalEvaluations_{};
    std::size_t totalReroutes_{};
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_DYNAMIC_ROUTING_MANAGER_H