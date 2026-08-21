#ifndef TRAFFICSIM_TRAFFIC_ROAD_TRAFFIC_MONITOR_H
#define TRAFFICSIM_TRAFFIC_ROAD_TRAFFIC_MONITOR_H

#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/traffic/RoadTrafficMetrics.h"
#include "trafficsim/vehicles/Vehicle.h"

#include <cstddef>
#include <span>
#include <unordered_map>
#include <vector>

namespace trafficsim
{

// MSVC's std::unordered_map move constructor may allocate and throw.
// Propagating that exception is safer than forcing a noexcept move.
// NOLINTNEXTLINE(bugprone-exception-escape)
class RoadTrafficMonitor final
{
  public:
    void update(const RoadNetwork &network, std::span<const Vehicle> vehicles);
    void reset() noexcept;

    [[nodiscard]] bool hasMetrics(RoadId roadId) const noexcept;
    [[nodiscard]] const RoadTrafficMetrics &metricsFor(RoadId roadId) const;
    [[nodiscard]] std::vector<RoadTrafficMetrics> allMetrics() const;
    [[nodiscard]] std::size_t roadCount() const noexcept;

  private:
    std::unordered_map<RoadId, RoadTrafficMetrics> metricsByRoad_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_TRAFFIC_ROAD_TRAFFIC_MONITOR_H