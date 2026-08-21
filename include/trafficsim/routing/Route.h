#ifndef TRAFFICSIM_ROUTING_ROUTE_H
#define TRAFFICSIM_ROUTING_ROUTE_H

#include "trafficsim/network/Types.h"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace trafficsim
{

class Route final
{
  public:
    Route(std::vector<RoadId> roadIds, double totalDistanceMeters);

    [[nodiscard]] std::span<const RoadId> segments() const noexcept;
    [[nodiscard]] std::span<const RoadId> remainingSegments() const noexcept;

    void replaceRemainingSegments(std::vector<RoadId> roadIds, double totalDistanceMeters);

    [[nodiscard]] std::optional<RoadId> currentRoad() const noexcept;
    [[nodiscard]] std::optional<RoadId> nextRoad() const noexcept;

    [[nodiscard]] bool advance() noexcept;
    [[nodiscard]] bool isComplete() const noexcept;

    [[nodiscard]] std::size_t segmentCount() const noexcept;
    [[nodiscard]] double totalDistanceMeters() const noexcept;

  private:
    std::vector<RoadId> roadIds_;
    std::size_t currentSegment_{};
    double totalDistanceMeters_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_ROUTING_ROUTE_H