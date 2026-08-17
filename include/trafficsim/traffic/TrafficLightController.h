#ifndef TRAFFICSIM_TRAFFIC_TRAFFIC_LIGHT_CONTROLLER_H
#define TRAFFICSIM_TRAFFIC_TRAFFIC_LIGHT_CONTROLLER_H

#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/traffic/TrafficLight.h"

#include <cstddef>
#include <unordered_map>

namespace trafficsim
{

// MSVC's std::unordered_map move constructor may allocate and throw.
// Propagating that exception is safer than forcing a noexcept move.
// NOLINTNEXTLINE(bugprone-exception-escape)
class TrafficLightController final
{
  public:
    explicit TrafficLightController(IntersectionId intersectionId);

    void addLight(const RoadNetwork &network, RoadId incomingRoadId, TrafficLight light);

    [[nodiscard]] bool hasLight(RoadId incomingRoadId) const noexcept;

    [[nodiscard]] const TrafficLight &getLight(RoadId incomingRoadId) const;

    [[nodiscard]] TrafficLightState stateForRoad(RoadId incomingRoadId) const;

    [[nodiscard]] bool allowsEntry(RoadId incomingRoadId) const;

    void update(double deltaSeconds);
    void reset() noexcept;

    [[nodiscard]] IntersectionId intersectionId() const noexcept;
    [[nodiscard]] std::size_t lightCount() const noexcept;

  private:
    IntersectionId intersectionId_;
    std::unordered_map<RoadId, TrafficLight> lights_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_TRAFFIC_TRAFFIC_LIGHT_CONTROLLER_H
