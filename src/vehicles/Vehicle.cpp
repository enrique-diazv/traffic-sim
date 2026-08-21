#include "trafficsim/vehicles/Vehicle.h"

#include "trafficsim/traffic/TrafficManager.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace trafficsim
{

namespace
{

void validateDynamics(const VehicleDynamics &dynamics)
{
    if (!std::isfinite(dynamics.maximumSpeedMetersPerSecond) ||
        dynamics.maximumSpeedMetersPerSecond <= 0.0)
    {
        throw std::invalid_argument{"Vehicle maximum speed must be finite and positive"};
    }

    if (!std::isfinite(dynamics.accelerationMetersPerSecondSquared) ||
        dynamics.accelerationMetersPerSecondSquared <= 0.0)
    {
        throw std::invalid_argument{"Vehicle acceleration must be finite and positive"};
    }

    if (!std::isfinite(dynamics.decelerationMetersPerSecondSquared) ||
        dynamics.decelerationMetersPerSecondSquared <= 0.0)
    {
        throw std::invalid_argument{"Vehicle deceleration must be finite and positive"};
    }
}

void validateRoute(const RoadNetwork &network, IntersectionId origin, IntersectionId destination,
                   const Route &route)
{
    const auto segments = route.segments();

    if (segments.empty())
    {
        if (origin != destination)
        {
            throw std::invalid_argument{"An empty route requires equal origin and destination"};
        }

        return;
    }

    if (origin == destination)
    {
        throw std::invalid_argument{"A non-empty route requires different origin and destination"};
    }

    if (route.isComplete() || !route.currentRoad().has_value() ||
        *route.currentRoad() != segments.front())
    {
        throw std::invalid_argument{"Vehicle route must not be advanced before starting"};
    }

    auto expectedOrigin = origin;

    for (const auto roadId : segments)
    {
        if (!network.hasRoad(roadId))
        {
            throw std::invalid_argument{"Vehicle route references a missing road"};
        }

        const auto &road = network.getRoad(roadId);

        if (road.origin() != expectedOrigin)
        {
            throw std::invalid_argument{"Vehicle route contains disconnected roads"};
        }

        expectedOrigin = road.destination();
    }

    if (expectedOrigin != destination)
    {
        throw std::invalid_argument{"Vehicle route does not reach its destination"};
    }
}

double calculateRouteDistance(const RoadNetwork &network, std::span<const RoadId> roadIds)
{
    double totalDistanceMeters = 0.0;

    for (const auto roadId : roadIds)
    {
        totalDistanceMeters += network.getRoad(roadId).lengthMeters();

        if (!std::isfinite(totalDistanceMeters))
        {
            throw std::overflow_error{"Vehicle route distance overflow"};
        }
    }

    return totalDistanceMeters;
}

} // namespace

void VehicleDynamics::validate() const
{
    validateDynamics(*this);
}

void VehicleFollowingConfig::validate() const
{
    if (!std::isfinite(minimumDistanceMeters) || minimumDistanceMeters < 0.0)
    {
        throw std::invalid_argument{"Minimum following distance must be finite and non-negative"};
    }

    if (!std::isfinite(reactionTimeSeconds) || reactionTimeSeconds <= 0.0)
    {
        throw std::invalid_argument{"Reaction time must be finite and positive"};
    }
}

void VehicleFollowingConstraint::validate(double currentPositionMeters) const
{
    if (!std::isfinite(maximumPositionMeters) || maximumPositionMeters < currentPositionMeters)
    {
        throw std::invalid_argument{
            "Following maximum position must be finite and not behind the vehicle"};
    }

    if (!std::isfinite(desiredSpeedLimitMetersPerSecond) || desiredSpeedLimitMetersPerSecond < 0.0)
    {
        throw std::invalid_argument{"Following speed limit must be finite and non-negative"};
    }
}

// Parameter names distinguish domain aliases that currently share convertible storage types.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Vehicle::Vehicle(VehicleId vehicleId, IntersectionId origin, IntersectionId destination,
                 Route route, VehicleDynamics dynamics)
    : id_{vehicleId}, origin_{origin}, destination_{destination}, route_{std::move(route)},
      dynamics_{dynamics}
{
    dynamics_.validate();
}

VehicleId Vehicle::id() const noexcept
{
    return id_;
}

IntersectionId Vehicle::origin() const noexcept
{
    return origin_;
}

IntersectionId Vehicle::destination() const noexcept
{
    return destination_;
}

VehicleState Vehicle::state() const noexcept
{
    return state_;
}

std::optional<RoadId> Vehicle::currentRoad() const noexcept
{
    return route_.currentRoad();
}

const Route &Vehicle::route() const noexcept
{
    return route_;
}

double Vehicle::positionMeters() const noexcept
{
    return positionMeters_;
}

double Vehicle::speedMetersPerSecond() const noexcept
{
    return speedMetersPerSecond_;
}

double Vehicle::maximumSpeedMetersPerSecond() const noexcept
{
    return dynamics_.maximumSpeedMetersPerSecond;
}

std::optional<double> Vehicle::spawnTimeSeconds() const noexcept
{
    return spawnTimeSeconds_;
}

std::optional<double> Vehicle::arrivalTimeSeconds() const noexcept
{
    return arrivalTimeSeconds_;
}

std::optional<double> Vehicle::travelTimeSeconds() const noexcept
{
    if (!spawnTimeSeconds_.has_value() || !arrivalTimeSeconds_.has_value())
    {
        return std::nullopt;
    }

    return *arrivalTimeSeconds_ - *spawnTimeSeconds_;
}

double Vehicle::waitingTimeSeconds() const noexcept
{
    return waitingTimeSeconds_;
}

bool Vehicle::start(const RoadNetwork &network, double spawnTimeSeconds)
{
    if (state_ != VehicleState::Spawning)
    {
        return false;
    }

    if (!std::isfinite(spawnTimeSeconds) || spawnTimeSeconds < 0.0)
    {
        throw std::invalid_argument{"Vehicle spawn time must be finite and non-negative"};
    }

    validateRoute(network, origin_, destination_, route_);

    spawnTimeSeconds_ = spawnTimeSeconds;
    state_ = route_.isComplete() ? VehicleState::Arrived : VehicleState::Driving;

    if (state_ == VehicleState::Arrived)
    {
        arrivalTimeSeconds_ = spawnTimeSeconds;
    }

    return true;
}

bool Vehicle::reroute(const RoadNetwork &network, Route continuation)
{
    switch (state_)
    {
    case VehicleState::Driving:
    case VehicleState::Waiting:
    case VehicleState::StoppedAtLight:
        break;

    case VehicleState::Spawning:
    case VehicleState::Rerouting:
    case VehicleState::Arrived:
        return false;
    }

    const auto currentRoadId = route_.currentRoad();

    if (!currentRoadId.has_value())
    {
        throw std::logic_error{"Active vehicle has no current road for rerouting"};
    }

    const auto &currentRoad = network.getRoad(*currentRoadId);

    validateRoute(network, currentRoad.destination(), destination_, continuation);

    const auto continuationSegments = continuation.segments();
    std::vector<RoadId> replacementRemainingSegments;
    replacementRemainingSegments.reserve(1 + continuationSegments.size());
    replacementRemainingSegments.push_back(*currentRoadId);
    replacementRemainingSegments.insert(replacementRemainingSegments.end(),
                                        continuationSegments.begin(), continuationSegments.end());

    const auto completedSegmentCount = route_.segmentCount() - route_.remainingSegments().size();
    const auto completedSegments = route_.segments().first(completedSegmentCount);

    const auto totalDistanceMeters = calculateRouteDistance(network, completedSegments) +
                                     calculateRouteDistance(network, replacementRemainingSegments);

    if (!std::isfinite(totalDistanceMeters))
    {
        throw std::overflow_error{"Rerouted vehicle distance overflow"};
    }

    route_.replaceRemainingSegments(std::move(replacementRemainingSegments), totalDistanceMeters);
    return true;
}

void Vehicle::resumeFromTrafficLight(const TrafficManager *trafficManager)
{
    if (state_ != VehicleState::StoppedAtLight)
    {
        return;
    }

    const auto stoppedRoadId = route_.currentRoad();

    if (!stoppedRoadId.has_value())
    {
        throw std::logic_error{"Vehicle stopped at light has no current road"};
    }

    if (trafficManager != nullptr && !trafficManager->allowsEntry(*stoppedRoadId))
    {
        return;
    }

    state_ = VehicleState::Driving;
}

void Vehicle::resumeFromQueue(const VehicleFollowingConstraint *followingConstraint)
{
    if (state_ != VehicleState::Waiting)
    {
        return;
    }

    if (followingConstraint != nullptr &&
        followingConstraint->maximumPositionMeters <= positionMeters_)
    {
        return;
    }

    state_ = VehicleState::Driving;
}

void Vehicle::updateSpeed(double desiredSpeed, double deltaSeconds) noexcept
{
    if (speedMetersPerSecond_ < desiredSpeed)
    {
        speedMetersPerSecond_ = std::min(
            desiredSpeed,
            speedMetersPerSecond_ + (dynamics_.accelerationMetersPerSecondSquared * deltaSeconds));
    }
    else if (speedMetersPerSecond_ > desiredSpeed)
    {
        speedMetersPerSecond_ = std::max(
            desiredSpeed,
            speedMetersPerSecond_ - (dynamics_.decelerationMetersPerSecondSquared * deltaSeconds));
    }
}

void Vehicle::applyFollowingConstraint(const VehicleFollowingConstraint *followingConstraint)
{
    if (followingConstraint == nullptr ||
        positionMeters_ < followingConstraint->maximumPositionMeters)
    {
        return;
    }

    positionMeters_ = followingConstraint->maximumPositionMeters;
    speedMetersPerSecond_ = 0.0;
    state_ = VehicleState::Waiting;
}

void Vehicle::advanceAcrossCompletedRoads(const RoadNetwork &network,
                                          const TrafficManager *trafficManager)
{
    while (state_ == VehicleState::Driving)
    {
        const auto activeRoadId = route_.currentRoad();

        if (!activeRoadId.has_value())
        {
            throw std::logic_error{"Driving vehicle has no current road"};
        }

        const auto &activeRoad = network.getRoad(*activeRoadId);

        if (positionMeters_ < activeRoad.lengthMeters())
        {
            break;
        }

        if (trafficManager != nullptr && !trafficManager->allowsEntry(*activeRoadId))
        {
            positionMeters_ = activeRoad.lengthMeters();
            speedMetersPerSecond_ = 0.0;
            state_ = VehicleState::StoppedAtLight;
            break;
        }

        positionMeters_ -= activeRoad.lengthMeters();

        if (!route_.advance())
        {
            throw std::logic_error{"Vehicle could not advance its route"};
        }

        if (route_.isComplete())
        {
            state_ = VehicleState::Arrived;
            arrivalTimeSeconds_ = *spawnTimeSeconds_ + elapsedTravelTimeSeconds_;
            positionMeters_ = 0.0;
            speedMetersPerSecond_ = 0.0;
        }
    }
}

void Vehicle::update(double deltaSeconds, const RoadNetwork &network,
                     const TrafficManager *trafficManager,
                     const VehicleFollowingConstraint *followingConstraint)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
    {
        throw std::invalid_argument{"Vehicle update duration must be finite and non-negative"};
    }

    if (followingConstraint != nullptr)
    {
        followingConstraint->validate(positionMeters_);
    }

    const auto tracksTravelTime =
        state_ != VehicleState::Spawning && state_ != VehicleState::Arrived;

    if (tracksTravelTime)
    {
        if (!spawnTimeSeconds_.has_value())
        {
            throw std::logic_error{"Active vehicle has no spawn time"};
        }

        const auto nextElapsedTravelTime = elapsedTravelTimeSeconds_ + deltaSeconds;
        const auto nextSimulationTime = *spawnTimeSeconds_ + nextElapsedTravelTime;

        if (!std::isfinite(nextElapsedTravelTime) || !std::isfinite(nextSimulationTime))
        {
            throw std::overflow_error{"Vehicle travel time overflow"};
        }

        elapsedTravelTimeSeconds_ = nextElapsedTravelTime;
    }

    resumeFromTrafficLight(trafficManager);
    resumeFromQueue(followingConstraint);

    if (state_ == VehicleState::Waiting || state_ == VehicleState::StoppedAtLight)
    {
        const auto nextWaitingTime = waitingTimeSeconds_ + deltaSeconds;

        if (!std::isfinite(nextWaitingTime))
        {
            throw std::overflow_error{"Vehicle waiting time overflow"};
        }

        waitingTimeSeconds_ = nextWaitingTime;
    }

    if (state_ != VehicleState::Driving || deltaSeconds == 0.0)
    {
        return;
    }

    const auto currentRoadId = route_.currentRoad();

    if (!currentRoadId.has_value())
    {
        throw std::logic_error{"Driving vehicle has no current road"};
    }

    const auto &road = network.getRoad(*currentRoadId);
    auto desiredSpeed =
        std::min(dynamics_.maximumSpeedMetersPerSecond, road.speedLimitMetersPerSecond());

    if (followingConstraint != nullptr)
    {
        desiredSpeed =
            std::min(desiredSpeed, followingConstraint->desiredSpeedLimitMetersPerSecond);
    }

    updateSpeed(desiredSpeed, deltaSeconds);

    const auto traveledDistance = speedMetersPerSecond_ * deltaSeconds;

    if (!std::isfinite(traveledDistance))
    {
        throw std::overflow_error{"Vehicle traveled distance overflow"};
    }

    positionMeters_ += traveledDistance;

    applyFollowingConstraint(followingConstraint);
    advanceAcrossCompletedRoads(network, trafficManager);
}

} // namespace trafficsim
