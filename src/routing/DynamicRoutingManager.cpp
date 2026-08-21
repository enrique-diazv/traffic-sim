#include "trafficsim/routing/DynamicRoutingManager.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace trafficsim
{

namespace
{

bool canEvaluateRoute(const Vehicle &vehicle) noexcept
{
    switch (vehicle.state())
    {
    case VehicleState::Driving:
    case VehicleState::Waiting:
    case VehicleState::StoppedAtLight:
        return true;

    case VehicleState::Spawning:
    case VehicleState::Rerouting:
    case VehicleState::Arrived:
        return false;
    }

    return false;
}

std::optional<double>
lastEvaluationTime(const std::unordered_map<VehicleId, double> &evaluationTimes,
                   VehicleId vehicleId)
{
    const auto iterator = evaluationTimes.find(vehicleId);

    return iterator == evaluationTimes.end() ? std::nullopt
                                             : std::optional<double>{iterator->second};
}

CongestionState worstCongestionState(std::span<const RoadId> roadIds,
                                     const RoadTrafficMonitor &trafficMonitor)
{
    auto worstState = CongestionState::FreeFlow;

    for (const auto roadId : roadIds)
    {
        if (!trafficMonitor.hasMetrics(roadId))
        {
            continue;
        }

        worstState = std::max(worstState, trafficMonitor.metricsFor(roadId).congestionState);
    }

    return worstState;
}

void checkedIncrement(std::size_t &value, const char *message)
{
    if (value == std::numeric_limits<std::size_t>::max())
    {
        throw std::overflow_error{message};
    }

    ++value;
}

} // namespace

DynamicRoutingManager::DynamicRoutingManager(ReroutingConfig reroutingConfig,
                                             CongestionCostConfig congestionCostConfig)
    : routePlanner_{CongestionAwareRouteCost{congestionCostConfig}},
      routeComparator_{CongestionAwareRouteCost{congestionCostConfig}},
      reroutingPolicy_{reroutingConfig}
{
}

DynamicRoutingResult DynamicRoutingManager::update(double simulationTimeSeconds,
                                                   const RoadNetwork &network,
                                                   const RoadTrafficMonitor &trafficMonitor,
                                                   VehicleManager &vehicleManager)
{
    std::unordered_set<VehicleId> activeVehicleIds;
    std::vector<VehicleId> candidateVehicleIds;

    activeVehicleIds.reserve(vehicleManager.vehicleCount());
    candidateVehicleIds.reserve(vehicleManager.vehicleCount());

    for (const auto &vehicle : vehicleManager.vehicles())
    {
        activeVehicleIds.insert(vehicle.id());

        if (canEvaluateRoute(vehicle))
        {
            candidateVehicleIds.push_back(vehicle.id());
        }
    }

    std::erase_if(lastEvaluationTimeByVehicle_, [&activeVehicleIds](const auto &entry)
                  { return !activeVehicleIds.contains(entry.first); });

    DynamicRoutingResult result;

    if (trafficMonitor.roadCount() == 0)
    {
        return result;
    }

    const auto roadMetrics = trafficMonitor.allMetrics();

    for (const auto vehicleId : candidateVehicleIds)
    {
        auto &vehicle = vehicleManager.getVehicle(vehicleId);
        const auto currentRoadId = vehicle.currentRoad();

        if (!currentRoadId.has_value())
        {
            throw std::logic_error{"Vehicle eligible for rerouting has no current road"};
        }

        const auto nextIntersection = network.getRoad(*currentRoadId).destination();

        if (nextIntersection == vehicle.destination())
        {
            continue;
        }

        const auto previousEvaluation = lastEvaluationTime(lastEvaluationTimeByVehicle_, vehicleId);

        if (!reroutingPolicy_.evaluationDue(simulationTimeSeconds, previousEvaluation))
        {
            continue;
        }

        lastEvaluationTimeByVehicle_[vehicleId] = simulationTimeSeconds;
        checkedIncrement(result.evaluatedVehicles, "Dynamic routing evaluation count overflow");
        checkedIncrement(totalEvaluations_, "Total dynamic routing evaluation count overflow");

        const auto remainingSegments = vehicle.route().remainingSegments();

        if (remainingSegments.size() < 2)
        {
            throw std::logic_error{"Vehicle route does not continue toward its destination"};
        }

        const auto currentContinuation = remainingSegments.subspan(1);
        auto candidateRoute = routePlanner_.calculateRoute(network, nextIntersection,
                                                           vehicle.destination(), roadMetrics);

        if (!candidateRoute.has_value())
        {
            continue;
        }

        const auto comparison = routeComparator_.compare(network, currentContinuation,
                                                         candidateRoute->segments(), roadMetrics);
        const auto worstState = worstCongestionState(currentContinuation, trafficMonitor);

        if (!reroutingPolicy_.shouldReroute(worstState, comparison))
        {
            continue;
        }

        if (!vehicle.reroute(network, std::move(*candidateRoute)))
        {
            throw std::logic_error{"Eligible vehicle rejected a dynamic route"};
        }

        checkedIncrement(result.reroutedVehicles, "Dynamic rerouting count overflow");
        checkedIncrement(totalReroutes_, "Total dynamic rerouting count overflow");
    }

    return result;
}

void DynamicRoutingManager::reset() noexcept
{
    lastEvaluationTimeByVehicle_.clear();
    totalEvaluations_ = 0;
    totalReroutes_ = 0;
}

std::size_t DynamicRoutingManager::totalEvaluations() const noexcept
{
    return totalEvaluations_;
}

std::size_t DynamicRoutingManager::totalReroutes() const noexcept
{
    return totalReroutes_;
}

} // namespace trafficsim