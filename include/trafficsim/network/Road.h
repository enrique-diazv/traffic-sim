#ifndef TRAFFICSIM_NETWORK_ROAD_H
#define TRAFFICSIM_NETWORK_ROAD_H

#include "trafficsim/network/Types.h"

#include <cstdint>

namespace trafficsim
{

struct RoadProperties
{
    IntersectionId origin;
    IntersectionId destination;
    double lengthMeters;
    double speedLimitMetersPerSecond;
    std::uint32_t laneCount;
    std::uint32_t capacity;
};

class Road final
{
  public:
    Road(RoadId roadId, RoadProperties properties);

    [[nodiscard]] RoadId id() const noexcept;
    [[nodiscard]] IntersectionId origin() const noexcept;
    [[nodiscard]] IntersectionId destination() const noexcept;
    [[nodiscard]] double lengthMeters() const noexcept;
    [[nodiscard]] double speedLimitMetersPerSecond() const noexcept;
    [[nodiscard]] std::uint32_t laneCount() const noexcept;
    [[nodiscard]] std::uint32_t capacity() const noexcept;

  private:
    RoadId id_;
    RoadProperties properties_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_NETWORK_ROAD_H