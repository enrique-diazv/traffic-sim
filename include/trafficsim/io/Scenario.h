#ifndef TRAFFICSIM_IO_SCENARIO_H
#define TRAFFICSIM_IO_SCENARIO_H

#include "trafficsim/core/SimulationConfig.h"
#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/traffic/TrafficManager.h"
#include "trafficsim/vehicles/VehicleSpawner.h"

#include <vector>

namespace trafficsim
{

struct Scenario
{
    SimulationConfig config;
    RoadNetwork roadNetwork;
    TrafficManager trafficManager;
    std::vector<VehicleSpawnRequest> spawnSchedule;
};

} // namespace trafficsim

#endif // TRAFFICSIM_IO_SCENARIO_H