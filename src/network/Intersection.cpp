#include "trafficsim/network/Intersection.h"

#include <cmath>
#include <stdexcept>

namespace trafficsim
{

Intersection::Intersection(IntersectionId intersectionId, Position position)
    : id_{intersectionId}, position_{position}
{
    if (!std::isfinite(position_.x) || !std::isfinite(position_.y))
    {
        throw std::invalid_argument{"Intersection position must be finite"};
    }
}

IntersectionId Intersection::id() const noexcept
{
    return id_;
}

Position Intersection::position() const noexcept
{
    return position_;
}

} // namespace trafficsim