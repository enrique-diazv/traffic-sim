#include "trafficsim/io/ScenarioLoader.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{

using trafficsim::ScenarioLoader;

std::string readBasicScenarioText()
{
    const auto filePath = std::filesystem::path{"scenarios"} / "basic.json";
    std::ifstream input{filePath, std::ios::binary};

    if (!input.is_open())
    {
        throw std::runtime_error{"Could not open basic scenario test fixture"};
    }

    return std::string{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };
}

TEST(ScenarioLoaderTests, LoadsSimulationConfiguration)
{
    constexpr std::string_view jsonText = R"json(
{
    "simulation": {
        "duration_seconds": 120.0,
        "time_step_seconds": 0.25,
        "random_seed": 12345,
        "maximum_vehicles": 250,
        "default_vehicle_dynamics": {
            "maximum_speed_meters_per_second": 15.0,
            "acceleration_meters_per_second_squared": 3.0,
            "deceleration_meters_per_second_squared": 5.0
        },
        "minimum_following_distance_meters": 2.5,
        "reaction_time_seconds": 1.25
    },
    "intersections": [],
    "roads": [],
    "traffic_lights": [],
    "vehicle_spawns": []
}
)json";

    const auto scenario = ScenarioLoader::loadFromJson(jsonText);
    const auto &config = scenario.config;

    EXPECT_DOUBLE_EQ(config.durationSeconds, 120.0);
    EXPECT_DOUBLE_EQ(config.timeStepSeconds, 0.25);
    EXPECT_EQ(config.randomSeed, 12345U);
    EXPECT_EQ(config.maximumVehicles, 250U);
    EXPECT_DOUBLE_EQ(config.defaultVehicleDynamics.maximumSpeedMetersPerSecond, 15.0);
    EXPECT_DOUBLE_EQ(config.defaultVehicleDynamics.accelerationMetersPerSecondSquared, 3.0);
    EXPECT_DOUBLE_EQ(config.defaultVehicleDynamics.decelerationMetersPerSecondSquared, 5.0);
    EXPECT_DOUBLE_EQ(config.minimumFollowingDistanceMeters, 2.5);
    EXPECT_DOUBLE_EQ(config.reactionTimeSeconds, 1.25);

    EXPECT_EQ(scenario.roadNetwork.intersectionCount(), 0U);
    EXPECT_EQ(scenario.roadNetwork.roadCount(), 0U);
    EXPECT_EQ(scenario.trafficManager.controllerCount(), 0U);
    EXPECT_TRUE(scenario.spawnSchedule.empty());
}

TEST(ScenarioLoaderTests, LoadsRoadNetwork)
{
    constexpr std::string_view jsonText = R"json(
{
    "simulation": {
        "duration_seconds": 60.0,
        "time_step_seconds": 0.1,
        "random_seed": 42,
        "maximum_vehicles": 100,
        "default_vehicle_dynamics": {
            "maximum_speed_meters_per_second": 13.9,
            "acceleration_meters_per_second_squared": 2.0,
            "deceleration_meters_per_second_squared": 4.0
        },
        "minimum_following_distance_meters": 2.0,
        "reaction_time_seconds": 1.0
    },
    "intersections": [
        {
            "id": 1,
            "position": {
                "x": 0.0,
                "y": 0.0
            }
        },
        {
            "id": 2,
            "position": {
                "x": 100.0,
                "y": 25.0
            }
        }
    ],
    "roads": [
        {
            "id": 10,
            "origin": 1,
            "destination": 2,
            "length_meters": 105.0,
            "speed_limit_meters_per_second": 13.9,
            "lane_count": 2,
            "capacity": 30
        }
    ],
    "traffic_lights": [],
    "vehicle_spawns": []
}
)json";

    const auto scenario = ScenarioLoader::loadFromJson(jsonText);
    const auto &network = scenario.roadNetwork;

    ASSERT_EQ(network.intersectionCount(), 2U);
    ASSERT_EQ(network.roadCount(), 1U);

    const auto firstPosition = network.getIntersection(1).position();
    EXPECT_DOUBLE_EQ(firstPosition.x, 0.0);
    EXPECT_DOUBLE_EQ(firstPosition.y, 0.0);

    const auto secondPosition = network.getIntersection(2).position();
    EXPECT_DOUBLE_EQ(secondPosition.x, 100.0);
    EXPECT_DOUBLE_EQ(secondPosition.y, 25.0);

    const auto &road = network.getRoad(10);
    EXPECT_EQ(road.origin(), 1U);
    EXPECT_EQ(road.destination(), 2U);
    EXPECT_DOUBLE_EQ(road.lengthMeters(), 105.0);
    EXPECT_DOUBLE_EQ(road.speedLimitMetersPerSecond(), 13.9);
    EXPECT_EQ(road.laneCount(), 2U);
    EXPECT_EQ(road.capacity(), 30U);

    const auto outgoingRoads = network.outgoingRoads(1);
    ASSERT_EQ(outgoingRoads.size(), 1U);
    EXPECT_EQ(outgoingRoads.front(), 10U);
}

TEST(ScenarioLoaderTests, LoadsTrafficLights)
{
    constexpr std::string_view jsonText = R"json(
{
    "simulation": {
        "duration_seconds": 60.0,
        "time_step_seconds": 0.1,
        "random_seed": 42,
        "maximum_vehicles": 100,
        "default_vehicle_dynamics": {
            "maximum_speed_meters_per_second": 13.9,
            "acceleration_meters_per_second_squared": 2.0,
            "deceleration_meters_per_second_squared": 4.0
        },
        "minimum_following_distance_meters": 2.0,
        "reaction_time_seconds": 1.0
    },
    "intersections": [
        {
            "id": 1,
            "position": {
                "x": 0.0,
                "y": 0.0
            }
        },
        {
            "id": 2,
            "position": {
                "x": 100.0,
                "y": 0.0
            }
        }
    ],
    "roads": [
        {
            "id": 10,
            "origin": 1,
            "destination": 2,
            "length_meters": 100.0,
            "speed_limit_meters_per_second": 13.9,
            "lane_count": 1,
            "capacity": 20
        }
    ],
    "traffic_lights": [
        {
            "intersection_id": 2,
            "lights": [
                {
                    "incoming_road_id": 10,
                    "initial_state": "red",
                    "timings": {
                        "green_seconds": 30.0,
                        "yellow_seconds": 5.0,
                        "red_seconds": 25.0
                    }
                }
            ]
        }
    ],
    "vehicle_spawns": []
}
)json";

    const auto scenario = ScenarioLoader::loadFromJson(jsonText);
    const auto &trafficManager = scenario.trafficManager;

    ASSERT_EQ(trafficManager.controllerCount(), 1U);
    ASSERT_TRUE(trafficManager.hasController(2));

    const auto &controller = trafficManager.getController(2);
    ASSERT_EQ(controller.lightCount(), 1U);
    ASSERT_TRUE(controller.hasLight(10));

    const auto &light = controller.getLight(10);
    EXPECT_EQ(light.state(), trafficsim::TrafficLightState::Red);
    EXPECT_DOUBLE_EQ(light.timings().greenSeconds, 30.0);
    EXPECT_DOUBLE_EQ(light.timings().yellowSeconds, 5.0);
    EXPECT_DOUBLE_EQ(light.timings().redSeconds, 25.0);
}

TEST(ScenarioLoaderTests, LoadsVehicleSpawnSchedule)
{
    constexpr std::string_view jsonText = R"json(
{
    "simulation": {
        "duration_seconds": 60.0,
        "time_step_seconds": 0.1,
        "random_seed": 42,
        "maximum_vehicles": 100,
        "default_vehicle_dynamics": {
            "maximum_speed_meters_per_second": 13.9,
            "acceleration_meters_per_second_squared": 2.0,
            "deceleration_meters_per_second_squared": 4.0
        },
        "minimum_following_distance_meters": 2.0,
        "reaction_time_seconds": 1.0
    },
    "intersections": [
        {
            "id": 1,
            "position": {
                "x": 0.0,
                "y": 0.0
            }
        },
        {
            "id": 2,
            "position": {
                "x": 100.0,
                "y": 0.0
            }
        }
    ],
    "roads": [
        {
            "id": 10,
            "origin": 1,
            "destination": 2,
            "length_meters": 100.0,
            "speed_limit_meters_per_second": 13.9,
            "lane_count": 1,
            "capacity": 20
        }
    ],
    "traffic_lights": [],
    "vehicle_spawns": [
        {
            "spawn_time_seconds": 0.0,
            "origin": 1,
            "destination": 2
        },
        {
            "spawn_time_seconds": 5.0,
            "origin": 2,
            "destination": 2
        }
    ]
}
)json";

    const auto scenario = ScenarioLoader::loadFromJson(jsonText);
    const auto &schedule = scenario.spawnSchedule;

    ASSERT_EQ(schedule.size(), 2U);

    EXPECT_DOUBLE_EQ(schedule[0].spawnTimeSeconds, 0.0);
    EXPECT_EQ(schedule[0].origin, 1U);
    EXPECT_EQ(schedule[0].destination, 2U);

    EXPECT_DOUBLE_EQ(schedule[1].spawnTimeSeconds, 5.0);
    EXPECT_EQ(schedule[1].origin, 2U);
    EXPECT_EQ(schedule[1].destination, 2U);
}

TEST(ScenarioLoaderTests, RejectsMalformedOrStructurallyInvalidJson)
{
    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson("{")), std::invalid_argument);

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson("[]")), std::invalid_argument);

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson("{}")), std::invalid_argument);
}

TEST(ScenarioLoaderTests, RejectsInvalidSimulationConfiguration)
{
    constexpr std::string_view jsonText = R"json(
{
    "simulation": {
        "duration_seconds": 0.0,
        "time_step_seconds": 0.1,
        "random_seed": 42,
        "maximum_vehicles": 100,
        "default_vehicle_dynamics": {
            "maximum_speed_meters_per_second": 13.9,
            "acceleration_meters_per_second_squared": 2.0,
            "deceleration_meters_per_second_squared": 4.0
        },
        "minimum_following_distance_meters": 2.0,
        "reaction_time_seconds": 1.0
    },
    "intersections": [],
    "roads": [],
    "traffic_lights": [],
    "vehicle_spawns": []
}
)json";

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson(jsonText)), std::invalid_argument);
}

TEST(ScenarioLoaderTests, LoadsCompleteScenarioFromFile)
{
    const auto filePath = std::filesystem::path{"scenarios"} / "basic.json";

    const auto scenario = ScenarioLoader::loadFromFile(filePath);

    EXPECT_DOUBLE_EQ(scenario.config.durationSeconds, 20.0);
    EXPECT_EQ(scenario.roadNetwork.intersectionCount(), 3U);
    EXPECT_EQ(scenario.roadNetwork.roadCount(), 2U);
    EXPECT_EQ(scenario.trafficManager.controllerCount(), 1U);
    EXPECT_EQ(scenario.spawnSchedule.size(), 3U);
}

TEST(ScenarioLoaderTests, RejectsMissingScenarioFile)
{
    const auto filePath = std::filesystem::path{"scenarios"} / "missing.json";

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromFile(filePath)), std::runtime_error);
}

TEST(ScenarioLoaderTests, RejectsRoadThatReferencesUnknownIntersection)
{
    auto jsonText = readBasicScenarioText();

    constexpr std::string_view original{"\"destination\": 2"};
    constexpr std::string_view replacement{"\"destination\": 99"};

    const auto position = jsonText.find(original);
    ASSERT_NE(position, std::string::npos);

    jsonText.replace(position, original.size(), replacement.data(), replacement.size());

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson(jsonText)), std::invalid_argument);
}

TEST(ScenarioLoaderTests, RejectsUnknownTrafficLightState)
{
    auto jsonText = readBasicScenarioText();

    constexpr std::string_view original{"\"initial_state\": \"green\""};
    constexpr std::string_view replacement{"\"initial_state\": \"blue\""};

    const auto position = jsonText.find(original);
    ASSERT_NE(position, std::string::npos);

    jsonText.replace(position, original.size(), replacement.data(), replacement.size());

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson(jsonText)), std::invalid_argument);
}

TEST(ScenarioLoaderTests, RejectsUnreachableVehicleSpawn)
{
    auto jsonText = readBasicScenarioText();

    constexpr std::string_view original{"\"origin\": 2"};
    constexpr std::string_view replacement{"\"origin\": 3"};

    const auto position = jsonText.find(original);
    ASSERT_NE(position, std::string::npos);

    jsonText.replace(position, original.size(), replacement.data(), replacement.size());

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson(jsonText)), std::invalid_argument);
}

TEST(ScenarioLoaderTests, RejectsSpawnAfterSimulationDuration)
{
    auto jsonText = readBasicScenarioText();

    constexpr std::string_view original{"\"spawn_time_seconds\": 2.0"};
    constexpr std::string_view replacement{"\"spawn_time_seconds\": 25.0"};

    const auto position = jsonText.find(original);
    ASSERT_NE(position, std::string::npos);

    jsonText.replace(position, original.size(), replacement.data(), replacement.size());

    EXPECT_THROW(static_cast<void>(ScenarioLoader::loadFromJson(jsonText)), std::invalid_argument);
}

} // namespace