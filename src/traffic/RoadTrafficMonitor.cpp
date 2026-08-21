#include "trafficsim/traffic/RoadTrafficMonitor.h"

#include "trafficsim/traffic/CongestionClassifier.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace trafficsim
{

void RoadTrafficMonitor::update(const RoadNetwork &network, std::span<const Vehicle> vehicles)
{
    std::unordered_map<RoadId, RoadTrafficObservation> observations;
    observations.reserve(network.roadCount());

    for (const auto &vehicle : vehicles)
    {
        const auto roadId = vehicle.currentRoad();

        if (!roadId.has_value())
        {
            continue;
        }

        if (!network.hasRoad(*roadId))
        {
            throw std::logic_error{"Vehicle references a road outside the monitored network"};
        }

        auto &observation = observations[*roadId];

        if (observation.vehicleCount == std::numeric_limits<std::size_t>::max())
        {
            throw std::overflow_error{"Road vehicle count overflow"};
        }

        ++observation.vehicleCount;

        const auto nextTotalSpeed =
            observation.totalSpeedMetersPerSecond + vehicle.speedMetersPerSecond();

        if (!std::isfinite(nextTotalSpeed))
        {
            throw std::overflow_error{"Road total vehicle speed overflow"};
        }

        observation.totalSpeedMetersPerSecond = nextTotalSpeed;
    }

    std::unordered_map<RoadId, RoadTrafficMetrics> nextMetrics;
    nextMetrics.reserve(network.roadCount());

    for (const auto roadId : network.roadIds())
    {
        const auto observationIterator = observations.find(roadId);
        const auto observation = observationIterator == observations.end()
                                     ? RoadTrafficObservation{}
                                     : observationIterator->second;

        nextMetrics.emplace(roadId,
                            CongestionClassifier::evaluate(network.getRoad(roadId), observation));
    }

    metricsByRoad_.swap(nextMetrics);
}

void RoadTrafficMonitor::reset() noexcept
{
    metricsByRoad_.clear();
}

bool RoadTrafficMonitor::hasMetrics(RoadId roadId) const noexcept
{
    return metricsByRoad_.contains(roadId);
}

const RoadTrafficMetrics &RoadTrafficMonitor::metricsFor(RoadId roadId) const
{
    const auto iterator = metricsByRoad_.find(roadId);

    if (iterator == metricsByRoad_.end())
    {
        throw std::out_of_range{"Road traffic metrics were not found"};
    }

    return iterator->second;
}

std::vector<RoadTrafficMetrics> RoadTrafficMonitor::allMetrics() const
{
    std::vector<RoadTrafficMetrics> metrics;
    metrics.reserve(metricsByRoad_.size());

    for (const auto &[roadId, roadMetrics] : metricsByRoad_)
    {
        static_cast<void>(roadId);
        metrics.push_back(roadMetrics);
    }

    std::ranges::sort(metrics, {}, &RoadTrafficMetrics::roadId);
    return metrics;
}

std::size_t RoadTrafficMonitor::roadCount() const noexcept
{
    return metricsByRoad_.size();
}

} // namespace trafficsim