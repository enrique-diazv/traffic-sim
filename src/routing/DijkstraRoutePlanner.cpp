#include "trafficsim/routing/DijkstraRoutePlanner.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace trafficsim
{

std::optional<Route> DijkstraRoutePlanner::calculateRoute(const RoadNetwork &network,
                                                          IntersectionId start,
                                                          IntersectionId destination) const
{
    if (!network.hasIntersection(start))
    {
        throw std::invalid_argument{"Route start intersection does not exist"};
    }

    if (!network.hasIntersection(destination))
    {
        throw std::invalid_argument{"Route destination intersection does not exist"};
    }

    if (start == destination)
    {
        return Route{std::vector<RoadId>{}, 0.0};
    }

    using QueueEntry = std::pair<double, IntersectionId>;

    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> frontier;

    std::unordered_map<IntersectionId, double> distanceByIntersection;
    std::unordered_map<IntersectionId, RoadId> predecessorRoadByIntersection;

    distanceByIntersection.emplace(start, 0.0);
    frontier.emplace(0.0, start);

    while (!frontier.empty())
    {
        const auto [currentDistance, currentIntersection] = frontier.top();
        frontier.pop();

        if (currentDistance > distanceByIntersection.at(currentIntersection))
        {
            continue;
        }

        if (currentIntersection == destination)
        {
            break;
        }

        for (const auto roadId : network.outgoingRoads(currentIntersection))
        {
            const auto &road = network.getRoad(roadId);
            const auto nextIntersection = road.destination();
            const auto candidateDistance = currentDistance + road.lengthMeters();

            if (!std::isfinite(candidateDistance))
            {
                throw std::overflow_error{"Route distance overflow"};
            }

            const auto knownDistance = distanceByIntersection.find(nextIntersection);

            if (knownDistance != distanceByIntersection.end() &&
                candidateDistance >= knownDistance->second)
            {
                continue;
            }

            distanceByIntersection[nextIntersection] = candidateDistance;
            predecessorRoadByIntersection[nextIntersection] = roadId;
            frontier.emplace(candidateDistance, nextIntersection);
        }
    }

    const auto destinationDistance = distanceByIntersection.find(destination);

    if (destinationDistance == distanceByIntersection.end())
    {
        return std::nullopt;
    }

    std::vector<RoadId> routeRoadIds;
    auto currentIntersection = destination;

    while (currentIntersection != start)
    {
        const auto predecessor = predecessorRoadByIntersection.find(currentIntersection);

        if (predecessor == predecessorRoadByIntersection.end())
        {
            throw std::logic_error{"Route reconstruction failed"};
        }

        const auto roadId = predecessor->second;
        routeRoadIds.push_back(roadId);
        currentIntersection = network.getRoad(roadId).origin();
    }

    std::ranges::reverse(routeRoadIds);

    return Route{std::move(routeRoadIds), destinationDistance->second};
}

} // namespace trafficsim