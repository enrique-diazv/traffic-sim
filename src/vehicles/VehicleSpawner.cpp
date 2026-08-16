#include "trafficsim/vehicles/VehicleSpawner.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace trafficsim
{

VehicleSpawner::VehicleSpawner(std::vector<VehicleSpawnRequest> schedule, VehicleDynamics dynamics,
                               VehicleId firstVehicleId)
    : schedule_{std::move(schedule)}, dynamics_{dynamics}, firstVehicleId_{firstVehicleId},
      nextVehicleId_{firstVehicleId}
{
    dynamics_.validate();

    for (const auto &request : schedule_)
    {
        if (!std::isfinite(request.spawnTimeSeconds) || request.spawnTimeSeconds < 0.0)
        {
            throw std::invalid_argument{"Vehicle spawn time must be finite and non-negative"};
        }
    }

    std::stable_sort(schedule_.begin(), schedule_.end(),
                     [](const VehicleSpawnRequest &left, const VehicleSpawnRequest &right)
                     { return left.spawnTimeSeconds < right.spawnTimeSeconds; });

    if (!schedule_.empty())
    {
        const auto requiredIncrements = schedule_.size() - 1;
        const auto availableIncrements = std::numeric_limits<VehicleId>::max() - firstVehicleId_;

        if (requiredIncrements > availableIncrements)
        {
            throw std::invalid_argument{"Vehicle spawn schedule exceeds the identifier range"};
        }
    }
}

std::size_t VehicleSpawner::spawnDue(double currentTimeSeconds, const RoadNetwork &network,
                                     const RoutePlanner &routePlanner,
                                     VehicleManager &vehicleManager)
{
    if (!std::isfinite(currentTimeSeconds) || currentTimeSeconds < 0.0)
    {
        throw std::invalid_argument{"Current spawn time must be finite and non-negative"};
    }

    std::size_t spawnedCount = 0;

    while (nextRequest_ < schedule_.size())
    {
        const auto &request = schedule_[nextRequest_];

        if (request.spawnTimeSeconds > currentTimeSeconds || vehicleManager.full())
        {
            break;
        }

        auto route = routePlanner.calculateRoute(network, request.origin, request.destination);

        if (!route.has_value())
        {
            throw std::runtime_error{"No route exists for the vehicle spawn request"};
        }

        Vehicle vehicle{
            nextVehicleId_, request.origin, request.destination, std::move(*route), dynamics_,
        };

        if (!vehicle.start(network))
        {
            throw std::logic_error{"Newly spawned vehicle could not be started"};
        }

        vehicleManager.addVehicle(std::move(vehicle));

        ++nextRequest_;
        ++spawnedCount;

        if (nextRequest_ < schedule_.size())
        {
            ++nextVehicleId_;
        }
    }

    return spawnedCount;
}

void VehicleSpawner::reset() noexcept
{
    nextVehicleId_ = firstVehicleId_;
    nextRequest_ = 0;
}

std::size_t VehicleSpawner::pendingCount() const noexcept
{
    return schedule_.size() - nextRequest_;
}

bool VehicleSpawner::complete() const noexcept
{
    return nextRequest_ == schedule_.size();
}

std::optional<double> VehicleSpawner::nextSpawnTime() const noexcept
{
    if (complete())
    {
        return std::nullopt;
    }

    return schedule_[nextRequest_].spawnTimeSeconds;
}

} // namespace trafficsim
