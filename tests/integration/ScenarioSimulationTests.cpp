#include "trafficsim/core/Simulation.h"
#include "trafficsim/io/ScenarioLoader.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <utility>

namespace
{

TEST(ScenarioSimulationTests, RunsCompleteScenarioLoadedFromFile)
{
    const auto filePath = std::filesystem::path{"scenarios"} / "basic.json";
    auto scenario = trafficsim::ScenarioLoader::loadFromFile(filePath);

    trafficsim::Simulation simulation{
        scenario.config,
        std::move(scenario.roadNetwork),
        std::move(scenario.spawnSchedule),
        std::move(scenario.trafficManager),
    };

    simulation.run();

    EXPECT_TRUE(simulation.finished());
    EXPECT_DOUBLE_EQ(simulation.clock().currentTimeSeconds(), 20.0);
    EXPECT_EQ(simulation.totalSpawnedVehicles(), 3U);
    EXPECT_EQ(simulation.totalArrivedVehicles(), 3U);
    EXPECT_TRUE(simulation.vehicleManager().empty());

    const auto summary = simulation.statistics().summary();

    EXPECT_EQ(summary.vehiclesSpawned, 3U);
    EXPECT_EQ(summary.vehiclesArrived, 3U);
}

} // namespace