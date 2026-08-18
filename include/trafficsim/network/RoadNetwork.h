#ifndef TRAFFICSIM_NETWORK_ROAD_NETWORK_H
#define TRAFFICSIM_NETWORK_ROAD_NETWORK_H

#include "trafficsim/network/Intersection.h"
#include "trafficsim/network/Road.h"

#include <cstddef>
#include <span>
#include <unordered_map>
#include <vector>

namespace trafficsim
{

// MSVC's std::unordered_map move constructor may allocate and throw.
// Propagating that exception is safer than forcing a noexcept move.
// NOLINTNEXTLINE(bugprone-exception-escape)
class RoadNetwork final
{
  public:
    void addIntersection(Intersection intersection);
    void addRoad(Road road);

    [[nodiscard]] bool hasIntersection(IntersectionId intersectionId) const noexcept;
    [[nodiscard]] bool hasRoad(RoadId roadId) const noexcept;

    [[nodiscard]] const Intersection &getIntersection(IntersectionId intersectionId) const;
    [[nodiscard]] const Road &getRoad(RoadId roadId) const;

    [[nodiscard]] std::span<const RoadId> outgoingRoads(IntersectionId intersectionId) const;
    [[nodiscard]] std::vector<RoadId> roadIds() const;

    [[nodiscard]] std::size_t intersectionCount() const noexcept;
    [[nodiscard]] std::size_t roadCount() const noexcept;

  private:
    std::unordered_map<IntersectionId, Intersection> intersections_;
    std::unordered_map<RoadId, Road> roads_;
    std::unordered_map<IntersectionId, std::vector<RoadId>> outgoingRoads_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_NETWORK_ROAD_NETWORK_H