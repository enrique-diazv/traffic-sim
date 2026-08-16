#ifndef TRAFFICSIM_VEHICLES_VEHICLE_MANAGER_H
#define TRAFFICSIM_VEHICLES_VEHICLE_MANAGER_H

#include "trafficsim/vehicles/Vehicle.h"

#include <cstddef>
#include <span>
#include <vector>

namespace trafficsim
{

class VehicleManager final
{
  public:
    explicit VehicleManager(std::size_t maximumVehicles);

    void addVehicle(Vehicle vehicle);

    [[nodiscard]] bool hasVehicle(VehicleId vehicleId) const noexcept;

    [[nodiscard]] Vehicle &getVehicle(VehicleId vehicleId);
    [[nodiscard]] const Vehicle &getVehicle(VehicleId vehicleId) const;

    [[nodiscard]] std::span<const Vehicle> vehicles() const noexcept;

    void update(double deltaSeconds, const RoadNetwork &network);
    [[nodiscard]] std::size_t removeArrived();
    void clear() noexcept;

    [[nodiscard]] std::size_t vehicleCount() const noexcept;
    [[nodiscard]] std::size_t maximumVehicles() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool full() const noexcept;

  private:
    std::vector<Vehicle> vehicles_;
    std::size_t maximumVehicles_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_VEHICLES_VEHICLE_MANAGER_H
