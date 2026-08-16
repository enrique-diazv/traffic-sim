#ifndef TRAFFICSIM_VEHICLES_VEHICLE_SPAWNER_H
#define TRAFFICSIM_VEHICLES_VEHICLE_SPAWNER_H

#include "trafficsim/routing/RoutePlanner.h"
#include "trafficsim/vehicles/VehicleManager.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace trafficsim
{

struct VehicleSpawnRequest
{
    double spawnTimeSeconds;
    IntersectionId origin;
    IntersectionId destination;
};

class VehicleSpawner final
{
  public:
    VehicleSpawner(std::vector<VehicleSpawnRequest> schedule, VehicleDynamics dynamics,
                   VehicleId firstVehicleId = 1);

    [[nodiscard]] std::size_t spawnDue(double currentTimeSeconds, const RoadNetwork &network,
                                       const RoutePlanner &routePlanner,
                                       VehicleManager &vehicleManager);

    void reset() noexcept;

    [[nodiscard]] std::size_t pendingCount() const noexcept;
    [[nodiscard]] bool complete() const noexcept;
    [[nodiscard]] std::optional<double> nextSpawnTime() const noexcept;

  private:
    std::vector<VehicleSpawnRequest> schedule_;
    VehicleDynamics dynamics_;
    VehicleId firstVehicleId_;
    VehicleId nextVehicleId_;
    std::size_t nextRequest_{};
};

} // namespace trafficsim

#endif // TRAFFICSIM_VEHICLES_VEHICLE_SPAWNER_H
