#ifndef TRAFFICSIM_TRAFFIC_TRAFFIC_MANAGER_H
#define TRAFFICSIM_TRAFFIC_TRAFFIC_MANAGER_H

#include "trafficsim/traffic/TrafficLightController.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace trafficsim
{

class TrafficManager final
{
  public:
    void addController(const RoadNetwork &network, TrafficLightController controller);

    [[nodiscard]] bool hasController(IntersectionId intersectionId) const noexcept;

    [[nodiscard]] const TrafficLightController &getController(IntersectionId intersectionId) const;

    [[nodiscard]] std::optional<TrafficLightState> stateForRoad(RoadId incomingRoadId) const;

    [[nodiscard]] bool allowsEntry(RoadId incomingRoadId) const;

    void update(double deltaSeconds);
    void reset() noexcept;

    [[nodiscard]] std::size_t controllerCount() const noexcept;

  private:
    std::vector<TrafficLightController> controllers_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_TRAFFIC_TRAFFIC_MANAGER_H
