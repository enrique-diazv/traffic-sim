#include "trafficsim/vehicles/VehicleManager.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace trafficsim
{

VehicleManager::VehicleManager(std::size_t maximumVehicles) : maximumVehicles_{maximumVehicles}
{
    if (maximumVehicles_ == 0)
    {
        throw std::invalid_argument{"Vehicle manager capacity must be greater than zero"};
    }
}

void VehicleManager::addVehicle(Vehicle vehicle)
{
    if (hasVehicle(vehicle.id()))
    {
        throw std::invalid_argument{"Vehicle manager already contains the vehicle identifier"};
    }

    if (full())
    {
        throw std::length_error{"Vehicle manager has reached its capacity"};
    }

    vehicles_.push_back(std::move(vehicle));
}

bool VehicleManager::hasVehicle(VehicleId vehicleId) const noexcept
{
    return std::ranges::any_of(vehicles_, [vehicleId](const Vehicle &vehicle)
                               { return vehicle.id() == vehicleId; });
}

Vehicle &VehicleManager::getVehicle(VehicleId vehicleId)
{
    const auto vehicle = std::ranges::find_if(vehicles_, [vehicleId](const Vehicle &candidate)
                                              { return candidate.id() == vehicleId; });

    if (vehicle == vehicles_.end())
    {
        throw std::out_of_range{"Vehicle identifier was not found"};
    }

    return *vehicle;
}

const Vehicle &VehicleManager::getVehicle(VehicleId vehicleId) const
{
    const auto vehicle = std::ranges::find_if(vehicles_, [vehicleId](const Vehicle &candidate)
                                              { return candidate.id() == vehicleId; });

    if (vehicle == vehicles_.end())
    {
        throw std::out_of_range{"Vehicle identifier was not found"};
    }

    return *vehicle;
}

std::span<const Vehicle> VehicleManager::vehicles() const noexcept
{
    return {vehicles_.data(), vehicles_.size()};
}

void VehicleManager::update(double deltaSeconds, const RoadNetwork &network,
                            const TrafficManager *trafficManager)
{
    for (auto &vehicle : vehicles_)
    {
        vehicle.update(deltaSeconds, network, trafficManager);
    }
}

std::size_t VehicleManager::removeArrived()
{
    return std::erase_if(vehicles_, [](const Vehicle &vehicle)
                         { return vehicle.state() == VehicleState::Arrived; });
}

void VehicleManager::clear() noexcept
{
    vehicles_.clear();
}

std::size_t VehicleManager::vehicleCount() const noexcept
{
    return vehicles_.size();
}

std::size_t VehicleManager::maximumVehicles() const noexcept
{
    return maximumVehicles_;
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
