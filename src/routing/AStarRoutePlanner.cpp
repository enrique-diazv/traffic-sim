#include "trafficsim/routing/AStarRoutePlanner.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace trafficsim
{

namespace
{

using MetricsByRoad = std::unordered_map<RoadId, const RoadTrafficMetrics *>;

MetricsByRoad indexMetrics(const RoadNetwork &network,
                           std::span<const RoadTrafficMetrics> roadMetrics)
{
    MetricsByRoad metricsByRoad;
    metricsByRoad.reserve(roadMetrics.size());

    for (const auto &metrics : roadMetrics)
    {
        if (!network.hasRoad(metrics.roadId))
        {
            throw std::invalid_argument{"Route metrics reference a missing road"};
        }

        const auto [iterator, inserted] = metricsByRoad.emplace(metrics.roadId, &metrics);
        static_cast<void>(iterator);

        if (!inserted)
        {
            throw std::invalid_argument{"Route metrics contain a duplicate road"};
        }
    }

    return metricsByRoad;
}

double geometricDistance(Position first, Position second)
{
    const auto distance = std::hypot(second.x - first.x, second.y - first.y);

    if (!std::isfinite(distance))
    {
        throw std::overflow_error{"Route heuristic distance overflow"};
    }

    return distance;
}

double calculateHeuristicScale(const RoadNetwork &network,
                               const CongestionAwareRouteCost &routeCost)
{
    auto minimumCostPerMeter = std::numeric_limits<double>::infinity();

    for (const auto roadId : network.roadIds())
    {
        const auto &road = network.getRoad(roadId);
        const auto origin = network.getIntersection(road.origin()).position();
        const auto destination = network.getIntersection(road.destination()).position();
        const auto distance = geometricDistance(origin, destination);

        if (distance == 0.0)
        {
            continue;
        }

        const auto costPerMeter = routeCost.calculate(road) / distance;

        if (!std::isfinite(costPerMeter))
        {
            throw std::overflow_error{"Route heuristic scale overflow"};
        }

        minimumCostPerMeter = std::min(minimumCostPerMeter, costPerMeter);
    }

    return std::isfinite(minimumCostPerMeter) ? minimumCostPerMeter : 0.0;
}

// The parameter names distinguish two intersection identifiers.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
double calculateHeuristic(const RoadNetwork &network, IntersectionId current,
                          IntersectionId destination, double costPerMeter)
{
    const auto currentPosition = network.getIntersection(current).position();
    const auto destinationPosition = network.getIntersection(destination).position();
    const auto heuristic = geometricDistance(currentPosition, destinationPosition) * costPerMeter;

    if (!std::isfinite(heuristic))
    {
        throw std::overflow_error{"Route heuristic cost overflow"};
    }

    return heuristic;
}

double calculateRoadCost(const Road &road, const MetricsByRoad &metricsByRoad,
                         const CongestionAwareRouteCost &routeCost)
{
    const auto metrics = metricsByRoad.find(road.id());

    return metrics == metricsByRoad.end() ? routeCost.calculate(road)
                                          : routeCost.calculate(road, *metrics->second);
}

} // namespace

AStarRoutePlanner::AStarRoutePlanner(CongestionAwareRouteCost routeCost) : routeCost_{routeCost} {}

std::optional<Route> AStarRoutePlanner::calculateRoute(const RoadNetwork &network,
                                                       IntersectionId start,
                                                       IntersectionId destination) const
{
    return calculateRoute(network, start, destination, {});
}

std::optional<Route>
AStarRoutePlanner::calculateRoute(const RoadNetwork &network, IntersectionId start,
                                  IntersectionId destination,
                                  std::span<const RoadTrafficMetrics> roadMetrics) const
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

    const auto metricsByRoad = indexMetrics(network, roadMetrics);
    const auto heuristicScale = calculateHeuristicScale(network, routeCost_);

    using QueueEntry = std::tuple<double, double, IntersectionId>;

    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> frontier;
    std::unordered_map<IntersectionId, double> costByIntersection;
    std::unordered_map<IntersectionId, RoadId> predecessorRoadByIntersection;

    costByIntersection.emplace(start, 0.0);
    frontier.emplace(calculateHeuristic(network, start, destination, heuristicScale), 0.0, start);

    while (!frontier.empty())
    {
        const auto [estimatedTotalCost, currentCost, currentIntersection] = frontier.top();
        static_cast<void>(estimatedTotalCost);
        frontier.pop();

        if (currentCost > costByIntersection.at(currentIntersection))
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
            const auto candidateCost =
                currentCost + calculateRoadCost(road, metricsByRoad, routeCost_);

            if (!std::isfinite(candidateCost))
            {
                throw std::overflow_error{"Route cost overflow"};
            }

            const auto knownCost = costByIntersection.find(nextIntersection);

            if (knownCost != costByIntersection.end() && candidateCost >= knownCost->second)
            {
                continue;
            }

            const auto estimatedCost =
                candidateCost +
                calculateHeuristic(network, nextIntersection, destination, heuristicScale);

            if (!std::isfinite(estimatedCost))
            {
                throw std::overflow_error{"Estimated route cost overflow"};
            }

            costByIntersection[nextIntersection] = candidateCost;
            predecessorRoadByIntersection[nextIntersection] = roadId;
            frontier.emplace(estimatedCost, candidateCost, nextIntersection);
        }
    }

    if (!costByIntersection.contains(destination))
    {
        return std::nullopt;
    }

    std::vector<RoadId> routeRoadIds;
    auto currentIntersection = destination;
    double totalDistanceMeters = 0.0;

    while (currentIntersection != start)
    {
        const auto predecessor = predecessorRoadByIntersection.find(currentIntersection);

        if (predecessor == predecessorRoadByIntersection.end())
        {
            throw std::logic_error{"Route reconstruction failed"};
        }

        const auto roadId = predecessor->second;
        const auto &road = network.getRoad(roadId);

        routeRoadIds.push_back(roadId);
        totalDistanceMeters += road.lengthMeters();

        if (!std::isfinite(totalDistanceMeters))
        {
            throw std::overflow_error{"Route distance overflow"};
        }

        currentIntersection = road.origin();
    }

    std::ranges::reverse(routeRoadIds);

    return Route{std::move(routeRoadIds), totalDistanceMeters};
}

} // namespace trafficsim