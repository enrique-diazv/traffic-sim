#include "trafficsim/network/Road.h"

#include <cmath>
#include <stdexcept>

namespace trafficsim
{

namespace
{

void validateRoadProperties(const RoadProperties &properties)
{
    if (properties.origin == properties.destination)
    {
        throw std::invalid_argument{"Road origin and destination must be different"};
    }

    if (!std::isfinite(properties.lengthMeters) || properties.lengthMeters <= 0.0)
    {
        throw std::invalid_argument{"Road length must be finite and greater than zero"};
    }

    if (!std::isfinite(properties.speedLimitMetersPerSecond) ||
        properties.speedLimitMetersPerSecond <= 0.0)
    {
        throw std::invalid_argument{"Road speed limit must be finite and greater than zero"};
    }

    if (properties.laneCount == 0)
    {
        throw std::invalid_argument{"Road lane count must be greater than zero"};
    }

    if (properties.capacity == 0)
    {
        throw std::invalid_argument{"Road capacity must be greater than zero"};
    }
}

} // namespace

Road::Road(RoadId roadId, RoadProperties properties) : id_{roadId}, properties_{properties}
{
    validateRoadProperties(properties_);
}

RoadId Road::id() const noexcept
{
    return id_;
}

IntersectionId Road::origin() const noexcept
{
    return properties_.origin;
}

IntersectionId Road::destination() const noexcept
{
    return properties_.destination;
}

double Road::lengthMeters() const noexcept
{
    return properties_.lengthMeters;
}

double Road::speedLimitMetersPerSecond() const noexcept
{
    return properties_.speedLimitMetersPerSecond;
}

std::uint32_t Road::laneCount() const noexcept
{
    return properties_.laneCount;
}

std::uint32_t Road::capacity() const noexcept
{
    return properties_.capacity;
}

} // namespace trafficsim