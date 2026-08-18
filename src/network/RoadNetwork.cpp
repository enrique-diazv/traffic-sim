#include "trafficsim/network/RoadNetwork.h"

#include <algorithm>
#include <stdexcept>

namespace trafficsim
{

void RoadNetwork::addIntersection(Intersection intersection)
{
    const auto intersectionId = intersection.id();

    if (hasIntersection(intersectionId))
    {
        throw std::invalid_argument{"Intersection ID already exists"};
    }

    intersections_.emplace(intersectionId, intersection);
    outgoingRoads_.emplace(intersectionId, std::vector<RoadId>{});
}

void RoadNetwork::addRoad(Road road)
{
    const auto roadId = road.id();

    if (hasRoad(roadId))
    {
        throw std::invalid_argument{"Road ID already exists"};
    }

    if (!hasIntersection(road.origin()))
    {
        throw std::invalid_argument{"Road origin does not exist"};
    }

    if (!hasIntersection(road.destination()))
    {
        throw std::invalid_argument{"Road destination does not exist"};
    }

    const auto origin = road.origin();
    roads_.emplace(roadId, road);

    try
    {
        outgoingRoads_.at(origin).push_back(roadId);
    }
    catch (...)
    {
        roads_.erase(roadId);
        throw;
    }
}

bool RoadNetwork::hasIntersection(IntersectionId intersectionId) const noexcept
{
    return intersections_.contains(intersectionId);
}

bool RoadNetwork::hasRoad(RoadId roadId) const noexcept
{
    return roads_.contains(roadId);
}

const Intersection &RoadNetwork::getIntersection(IntersectionId intersectionId) const
{
    const auto intersection = intersections_.find(intersectionId);

    if (intersection == intersections_.end())
    {
        throw std::out_of_range{"Intersection ID does not exist"};
    }

    return intersection->second;
}

const Road &RoadNetwork::getRoad(RoadId roadId) const
{
    const auto road = roads_.find(roadId);

    if (road == roads_.end())
    {
        throw std::out_of_range{"Road ID does not exist"};
    }

    return road->second;
}

std::span<const RoadId> RoadNetwork::outgoingRoads(IntersectionId intersectionId) const
{
    const auto roads = outgoingRoads_.find(intersectionId);

    if (roads == outgoingRoads_.end())
    {
        throw std::out_of_range{"Intersection ID does not exist"};
    }

    return std::span<const RoadId>{roads->second};
}

std::vector<RoadId> RoadNetwork::roadIds() const
{
    std::vector<RoadId> identifiers;
    identifiers.reserve(roads_.size());

    for (const auto &[roadId, road] : roads_)
    {
        static_cast<void>(road);
        identifiers.push_back(roadId);
    }

    std::ranges::sort(identifiers);
    return identifiers;
}

std::size_t RoadNetwork::intersectionCount() const noexcept
{
    return intersections_.size();
}

std::size_t RoadNetwork::roadCount() const noexcept
{
    return roads_.size();
}

} // namespace trafficsim