#include "trafficsim/vehicles/Vehicle.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

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

} // namespace

Vehicle::Vehicle(VehicleId vehicleId, IntersectionId origin, IntersectionId destination,
                 Route route, VehicleDynamics dynamics)
    : id_{vehicleId}, origin_{origin}, destination_{destination}, route_{std::move(route)},
      dynamics_{dynamics}
{
    validateDynamics(dynamics_);
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

bool Vehicle::start(const RoadNetwork &network)
{
    if (state_ != VehicleState::Spawning)
    {
        return false;
    }

    validateRoute(network, origin_, destination_, route_);

    state_ = route_.isComplete() ? VehicleState::Arrived : VehicleState::Driving;
    return true;
}

void Vehicle::update(double deltaSeconds, const RoadNetwork &network)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
    {
        throw std::invalid_argument{"Vehicle update duration must be finite and non-negative"};
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
    const auto desiredSpeed =
        std::min(dynamics_.maximumSpeedMetersPerSecond, road.speedLimitMetersPerSecond());

    if (speedMetersPerSecond_ < desiredSpeed)
    {
        speedMetersPerSecond_ =
            std::min(desiredSpeed, speedMetersPerSecond_ +
                                       dynamics_.accelerationMetersPerSecondSquared * deltaSeconds);
    }
    else if (speedMetersPerSecond_ > desiredSpeed)
    {
        speedMetersPerSecond_ =
            std::max(desiredSpeed, speedMetersPerSecond_ -
                                       dynamics_.decelerationMetersPerSecondSquared * deltaSeconds);
    }

    const auto traveledDistance = speedMetersPerSecond_ * deltaSeconds;

    if (!std::isfinite(traveledDistance))
    {
        throw std::overflow_error{"Vehicle traveled distance overflow"};
    }

    positionMeters_ += traveledDistance;

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

        positionMeters_ -= activeRoad.lengthMeters();

        if (!route_.advance())
        {
            throw std::logic_error{"Vehicle could not advance its route"};
        }

        if (route_.isComplete())
        {
            state_ = VehicleState::Arrived;
            positionMeters_ = 0.0;
            speedMetersPerSecond_ = 0.0;
        }
    }
}

} // namespace trafficsim