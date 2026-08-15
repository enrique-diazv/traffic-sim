#ifndef TRAFFICSIM_NETWORK_INTERSECTION_H
#define TRAFFICSIM_NETWORK_INTERSECTION_H
#include "trafficsim/network/Types.h"

namespace trafficsim
{

struct Position
{
    double x{};
    double y{};

    bool operator==(const Position &) const = default;
};

class Intersection final
{
  public:
    Intersection(IntersectionId intersectionId, Position position);

    [[nodiscard]] IntersectionId id() const noexcept;
    [[nodiscard]] Position position() const noexcept;

  private:
    IntersectionId id_;
    Position position_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_NETWORK_INTERSECTION_H