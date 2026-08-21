#include "trafficsim/core/Simulation.h"

#include <stdexcept>
#include <utility>

namespace trafficsim
{

Simulation::Simulation(SimulationConfig config, RoadNetwork network,
                       std::vector<VehicleSpawnRequest> spawnSchedule,
                       TrafficManager trafficManager)
    : config_{config}, roadNetwork_{std::move(network)}, clock_{config_.timeStepSeconds},
      trafficManager_{std::move(trafficManager)},
      vehicleManager_{
          config_.maximumVehicles,
          VehicleFollowingConfig{
              .minimumDistanceMeters = config_.minimumFollowingDistanceMeters,
              .reactionTimeSeconds = config_.reactionTimeSeconds,
          },
      },
      vehicleSpawner_{std::move(spawnSchedule), config_.defaultVehicleDynamics}
{
    config_.validate();
}

void Simulation::step()
{
    if (finished())
    {
        throw std::logic_error{"Cannot step a finished simulation"};
    }

    const auto spawnedCount = vehicleSpawner_.spawnDue(clock_.currentTimeSeconds(), roadNetwork_,
                                                       routePlanner_, vehicleManager_);

    totalSpawnedVehicles_ += spawnedCount;
    statisticsCollector_.recordSpawned(spawnedCount);
    statisticsCollector_.observeActiveVehicles(vehicleManager_.vehicles());

    vehicleManager_.update(config_.timeStepSeconds, roadNetwork_, &trafficManager_);

    roadTrafficMonitor_.update(roadNetwork_, vehicleManager_.vehicles());

    const auto roadMetrics = roadTrafficMonitor_.allMetrics();
    statisticsCollector_.observeRoads(config_.timeStepSeconds, roadMetrics);

    for (const auto &vehicle : vehicleManager_.vehicles())
    {
        if (vehicle.state() == VehicleState::Arrived)
        {
            statisticsCollector_.recordCompletedVehicle(vehicle);
        }
    }

    totalArrivedVehicles_ += vehicleManager_.removeArrived();

    trafficManager_.update(config_.timeStepSeconds);
    clock_.advance();
}

void Simulation::run()
{
    while (!finished())
    {
        step();
    }
}

void Simulation::reset() noexcept
{
    clock_.reset();
    trafficManager_.reset();
    vehicleManager_.clear();
    vehicleSpawner_.reset();
    roadTrafficMonitor_.reset();
    statisticsCollector_.reset();
    totalSpawnedVehicles_ = 0;
    totalArrivedVehicles_ = 0;
}

bool Simulation::finished() const noexcept
{
    return clock_.currentTimeSeconds() >= config_.durationSeconds;
}

const SimulationConfig &Simulation::config() const noexcept
{
    return config_;
}

const SimulationClock &Simulation::clock() const noexcept
{
    return clock_;
}

const RoadNetwork &Simulation::roadNetwork() const noexcept
{
    return roadNetwork_;
}

const TrafficManager &Simulation::trafficManager() const noexcept
{
    return trafficManager_;
}

const VehicleManager &Simulation::vehicleManager() const noexcept
{
    return vehicleManager_;
}

const RoadTrafficMonitor &Simulation::roadTrafficMonitor() const noexcept
{
    return roadTrafficMonitor_;
}

const StatisticsCollector &Simulation::statistics() const noexcept
{
    return statisticsCollector_;
}

std::size_t Simulation::totalSpawnedVehicles() const noexcept
{
    return totalSpawnedVehicles_;
}

std::size_t Simulation::totalArrivedVehicles() const noexcept
{
    return totalArrivedVehicles_;
}

} // namespace trafficsim
