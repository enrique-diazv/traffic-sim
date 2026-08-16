#ifndef TRAFFICSIM_CORE_SIMULATION_H
#define TRAFFICSIM_CORE_SIMULATION_H

#include "trafficsim/core/SimulationClock.h"
#include "trafficsim/core/SimulationConfig.h"
#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/routing/DijkstraRoutePlanner.h"
#include "trafficsim/vehicles/VehicleManager.h"
#include "trafficsim/vehicles/VehicleSpawner.h"

#include <cstddef>
#include <vector>

namespace trafficsim
{

class Simulation final
{
  public:
    Simulation(SimulationConfig config, RoadNetwork network,
               std::vector<VehicleSpawnRequest> spawnSchedule);

    void step();
    void run();
    void reset() noexcept;

    [[nodiscard]] bool finished() const noexcept;

    [[nodiscard]] const SimulationConfig &config() const noexcept;
    [[nodiscard]] const SimulationClock &clock() const noexcept;
    [[nodiscard]] const RoadNetwork &roadNetwork() const noexcept;
    [[nodiscard]] const VehicleManager &vehicleManager() const noexcept;

    [[nodiscard]] std::size_t totalSpawnedVehicles() const noexcept;
    [[nodiscard]] std::size_t totalArrivedVehicles() const noexcept;

  private:
    SimulationConfig config_;
    RoadNetwork roadNetwork_;
    SimulationClock clock_;
    DijkstraRoutePlanner routePlanner_;
    VehicleManager vehicleManager_;
    VehicleSpawner vehicleSpawner_;
    std::size_t totalSpawnedVehicles_{};
    std::size_t totalArrivedVehicles_{};
};

} // namespace trafficsim

#endif // TRAFFICSIM_CORE_SIMULATION_H
