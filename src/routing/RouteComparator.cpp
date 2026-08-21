#include "trafficsim/routing/RouteComparator.h"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <unordered_map>

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

double roadCost(const Road &road, const MetricsByRoad &metricsByRoad,
                const CongestionAwareRouteCost &routeCost)
{
    const auto metrics = metricsByRoad.find(road.id());

    return metrics == metricsByRoad.end() ? routeCost.calculate(road)
                                          : routeCost.calculate(road, *metrics->second);
}

} // namespace

RouteComparator::RouteComparator(CongestionAwareRouteCost routeCost) : routeCost_{routeCost} {}

double RouteComparator::calculateCost(const RoadNetwork &network, std::span<const RoadId> roadIds,
                                      std::span<const RoadTrafficMetrics> roadMetrics) const
{
    const auto metricsByRoad = indexMetrics(network, roadMetrics);
    std::optional<IntersectionId> expectedOrigin;
    double totalCost = 0.0;

    for (const auto roadId : roadIds)
    {
        if (!network.hasRoad(roadId))
        {
            throw std::invalid_argument{"Compared route references a missing road"};
        }

        const auto &road = network.getRoad(roadId);

        if (expectedOrigin.has_value() && road.origin() != *expectedOrigin)
        {
            throw std::invalid_argument{"Compared route contains disconnected roads"};
        }

        totalCost += roadCost(road, metricsByRoad, routeCost_);

        if (!std::isfinite(totalCost))
        {
            throw std::overflow_error{"Compared route cost overflow"};
        }

        expectedOrigin = road.destination();
    }

    return totalCost;
}

// The parameter names distinguish the current and candidate road sequences.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RouteComparisonResult
RouteComparator::compare(const RoadNetwork &network, std::span<const RoadId> currentRoadIds,
                         std::span<const RoadId> candidateRoadIds,
                         std::span<const RoadTrafficMetrics> roadMetrics) const
{
    const auto currentCost = calculateCost(network, currentRoadIds, roadMetrics);
    const auto candidateCost = calculateCost(network, candidateRoadIds, roadMetrics);

    const auto relativeImprovement =
        currentCost > 0.0 ? (currentCost - candidateCost) / currentCost : 0.0;

    if (!std::isfinite(relativeImprovement))
    {
        throw std::overflow_error{"Route comparison improvement overflow"};
    }

    return RouteComparisonResult{
        .currentCost = currentCost,
        .candidateCost = candidateCost,
        .relativeImprovement = relativeImprovement,
        .candidateIsBetter = candidateCost < currentCost,
    };
}

} // namespace trafficsim