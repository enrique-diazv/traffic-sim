#include "trafficsim/vehicles/VehicleManager.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>

namespace trafficsim
{

namespace
{

using RoadVehicleGroups = std::map<RoadId, std::vector<Vehicle *>>;

bool participatesInRoadTraffic(const Vehicle &vehicle) noexcept
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

bool isAhead(const Vehicle *left, const Vehicle *right) noexcept
{
    if (left->positionMeters() != right->positionMeters())
    {
        return left->positionMeters() > right->positionMeters();
    }

    return left->id() < right->id();
}

bool remainsOnRoad(const Vehicle &vehicle, RoadId roadId) noexcept
{
    const auto currentRoad = vehicle.currentRoad();

    return participatesInRoadTraffic(vehicle) && currentRoad.has_value() && *currentRoad == roadId;
}

} // namespace

VehicleManager::VehicleManager(std::size_t maximumVehicles, VehicleFollowingConfig followingConfig)
    : maximumVehicles_{maximumVehicles}, followingConfig_{followingConfig}
{
    if (maximumVehicles_ == 0)
    {
        throw std::invalid_argument{"Vehicle manager capacity must be greater than zero"};
    }

    followingConfig_.validate();
}

void VehicleManager::addVehicle(Vehicle vehicle)
{
    const auto vehicleId = vehicle.id();

    if (vehicleIndexById_.contains(vehicleId))
    {
        throw std::invalid_argument{"Vehicle manager already contains the vehicle identifier"};
    }

    if (full())
    {
        throw std::length_error{"Vehicle manager has reached its capacity"};
    }

    const auto vehicleIndex = vehicles_.size();
    const auto [indexIterator, inserted] = vehicleIndexById_.emplace(vehicleId, vehicleIndex);

    if (!inserted)
    {
        throw std::logic_error{"Vehicle identifier index became inconsistent"};
    }

    try
    {
        vehicles_.push_back(std::move(vehicle));
    }
    catch (...)
    {
        vehicleIndexById_.erase(indexIterator);
        throw;
    }
}

bool VehicleManager::hasVehicle(VehicleId vehicleId) const noexcept
{
    return vehicleIndexById_.contains(vehicleId);
}

Vehicle &VehicleManager::getVehicle(VehicleId vehicleId)
{
    const auto vehicleIndex = vehicleIndexById_.find(vehicleId);

    if (vehicleIndex == vehicleIndexById_.end())
    {
        throw std::out_of_range{"Vehicle identifier was not found"};
    }

    return vehicles_[vehicleIndex->second];
}

const Vehicle &VehicleManager::getVehicle(VehicleId vehicleId) const
{
    const auto vehicleIndex = vehicleIndexById_.find(vehicleId);

    if (vehicleIndex == vehicleIndexById_.end())
    {
        throw std::out_of_range{"Vehicle identifier was not found"};
    }

    return vehicles_[vehicleIndex->second];
}

std::span<const Vehicle> VehicleManager::vehicles() const noexcept
{
    return {vehicles_.data(), vehicles_.size()};
}

void VehicleManager::update(double deltaSeconds, const RoadNetwork &network,
                            const TrafficManager *trafficManager)
{
    RoadVehicleGroups vehiclesByRoad;

    for (auto &vehicle : vehicles_)
    {
        if (!participatesInRoadTraffic(vehicle))
        {
            continue;
        }

        const auto currentRoad = vehicle.currentRoad();

        if (!currentRoad.has_value())
        {
            throw std::logic_error{"Active vehicle has no current road"};
        }

        vehiclesByRoad[*currentRoad].push_back(&vehicle);
    }

    for (auto &[roadId, roadVehicles] : vehiclesByRoad)
    {
        std::ranges::sort(roadVehicles, isAhead);

        Vehicle *leader = nullptr;

        for (auto *vehicle : roadVehicles)
        {
            if (leader == nullptr)
            {
                vehicle->update(deltaSeconds, network, trafficManager);
            }
            else
            {
                const auto gapMeters =
                    std::max(0.0, leader->positionMeters() - vehicle->positionMeters());

                const auto safeDistanceMeters =
                    followingConfig_.minimumDistanceMeters +
                    (vehicle->speedMetersPerSecond() * followingConfig_.reactionTimeSeconds);

                const auto maximumPositionMeters = std::max(
                    vehicle->positionMeters(), leader->positionMeters() - safeDistanceMeters);

                const auto desiredSpeedLimit = gapMeters <= safeDistanceMeters
                                                   ? leader->speedMetersPerSecond()
                                                   : vehicle->maximumSpeedMetersPerSecond();

                const VehicleFollowingConstraint constraint{
                    .maximumPositionMeters = maximumPositionMeters,
                    .desiredSpeedLimitMetersPerSecond = desiredSpeedLimit,
                };

                vehicle->update(deltaSeconds, network, trafficManager, &constraint);
            }

            leader = remainsOnRoad(*vehicle, roadId) ? vehicle : nullptr;
        }
    }
}

std::size_t VehicleManager::removeArrived()
{
    for (const auto &vehicle : vehicles_)
    {
        if (vehicle.state() == VehicleState::Arrived)
        {
            vehicleIndexById_.erase(vehicle.id());
        }
    }

    const auto removedCount = std::erase_if(vehicles_, [](const Vehicle &vehicle)
                                            { return vehicle.state() == VehicleState::Arrived; });

    if (removedCount == 0U)
    {
        return 0U;
    }

    for (std::size_t index = 0U; index < vehicles_.size(); ++index)
    {
        const auto vehicleIndex = vehicleIndexById_.find(vehicles_[index].id());

        if (vehicleIndex == vehicleIndexById_.end())
        {
            throw std::logic_error{"Vehicle identifier index became inconsistent"};
        }

        vehicleIndex->second = index;
    }

    return removedCount;
}

void VehicleManager::clear() noexcept
{
    vehicles_.clear();
    vehicleIndexById_.clear();
}

std::size_t VehicleManager::vehicleCount() const noexcept
{
    return vehicles_.size();
}

std::size_t VehicleManager::maximumVehicles() const noexcept
{
    return maximumVehicles_;
}

const VehicleFollowingConfig &VehicleManager::followingConfig() const noexcept
{
    return followingConfig_;
}

bool VehicleManager::empty() const noexcept
{
    return vehicles_.empty();
}

bool VehicleManager::full() const noexcept
{
    return vehicles_.size() >= maximumVehicles_;
}

} // namespace trafficsim
