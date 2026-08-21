#include "trafficsim/io/ScenarioLoader.h"
#include "trafficsim/routing/DijkstraRoutePlanner.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace trafficsim
{

namespace
{

using Json = nlohmann::json;

[[noreturn]] void throwInvalidScenario(const std::string &message)
{
    throw std::invalid_argument{"Invalid scenario: " + message};
}

std::string qualifiedName(std::string_view context, std::string_view key)
{
    std::string result{context};
    result.push_back('.');
    result.append(key.data(), key.size());
    return result;
}

const Json &requiredField(const Json &object, std::string_view key, std::string_view context)
{
    const std::string keyText{key};
    const auto iterator = object.find(keyText);

    if (iterator == object.end())
    {
        throwInvalidScenario("missing required field " + qualifiedName(context, key));
    }

    return *iterator;
}

const Json &requiredObject(const Json &object, std::string_view key, std::string_view context)
{
    const auto &value = requiredField(object, key, context);

    if (!value.is_object())
    {
        throwInvalidScenario(qualifiedName(context, key) + " must be an object");
    }

    return value;
}

const Json *optionalObject(const Json &object, std::string_view key, std::string_view context)
{
    const std::string keyText{key};
    const auto iterator = object.find(keyText);

    if (iterator == object.end())
    {
        return nullptr;
    }

    if (!iterator->is_object())
    {
        throwInvalidScenario(qualifiedName(context, key) + " must be an object");
    }

    return &*iterator;
}

const Json &requiredArray(const Json &object, std::string_view key, std::string_view context)
{
    const auto &value = requiredField(object, key, context);

    if (!value.is_array())
    {
        throwInvalidScenario(qualifiedName(context, key) + " must be an array");
    }

    return value;
}

std::string indexedContext(std::string_view collection, std::size_t index)
{
    std::string result{collection};
    result.push_back('[');
    result += std::to_string(index);
    result.push_back(']');
    return result;
}

double requiredDouble(const Json &object, std::string_view key, std::string_view context)
{
    const auto &value = requiredField(object, key, context);

    if (!value.is_number())
    {
        throwInvalidScenario(qualifiedName(context, key) + " must be a number");
    }

    return value.get<double>();
}

std::string requiredString(const Json &object, std::string_view key, std::string_view context)
{
    const auto &value = requiredField(object, key, context);

    if (!value.is_string())
    {
        throwInvalidScenario(qualifiedName(context, key) + " must be a string");
    }

    return value.get<std::string>();
}

TrafficLightState parseTrafficLightState(std::string_view state, std::string_view context)
{
    if (state == "green")
    {
        return TrafficLightState::Green;
    }

    if (state == "yellow")
    {
        return TrafficLightState::Yellow;
    }

    if (state == "red")
    {
        return TrafficLightState::Red;
    }

    throwInvalidScenario(std::string{context} + " must be green, yellow, or red");
}

CongestionState parseCongestionState(std::string_view state, std::string_view context)
{
    if (state == "free_flow")
    {
        return CongestionState::FreeFlow;
    }

    if (state == "moderate")
    {
        return CongestionState::Moderate;
    }

    if (state == "congested")
    {
        return CongestionState::Congested;
    }

    if (state == "gridlock")
    {
        return CongestionState::Gridlock;
    }

    throwInvalidScenario(std::string{context} +
                         " must be free_flow, moderate, congested, or gridlock");
}

template <typename Unsigned>
Unsigned requiredUnsigned(const Json &object, std::string_view key, std::string_view context)
{
    static_assert(std::is_unsigned_v<Unsigned>);

    const auto &value = requiredField(object, key, context);

    if (!value.is_number_unsigned())
    {
        throwInvalidScenario(qualifiedName(context, key) + " must be an unsigned integer");
    }

    return value.get<Unsigned>();
}

RoadNetwork parseRoadNetwork(const Json &document)
{
    RoadNetwork network;

    const auto &intersections = requiredArray(document, "intersections", "root");

    for (std::size_t index = 0; index < intersections.size(); ++index)
    {
        const auto context = indexedContext("intersections", index);
        const auto &intersection = intersections[index];

        if (!intersection.is_object())
        {
            throwInvalidScenario(context + " must be an object");
        }

        const auto &position = requiredObject(intersection, "position", context);

        network.addIntersection(Intersection{
            requiredUnsigned<IntersectionId>(intersection, "id", context),
            Position{
                .x = requiredDouble(position, "x", context + ".position"),
                .y = requiredDouble(position, "y", context + ".position"),
            },
        });
    }

    const auto &roads = requiredArray(document, "roads", "root");

    for (std::size_t index = 0; index < roads.size(); ++index)
    {
        const auto context = indexedContext("roads", index);
        const auto &road = roads[index];

        if (!road.is_object())
        {
            throwInvalidScenario(context + " must be an object");
        }

        network.addRoad(Road{
            requiredUnsigned<RoadId>(road, "id", context),
            RoadProperties{
                .origin = requiredUnsigned<IntersectionId>(road, "origin", context),
                .destination = requiredUnsigned<IntersectionId>(road, "destination", context),
                .lengthMeters = requiredDouble(road, "length_meters", context),
                .speedLimitMetersPerSecond =
                    requiredDouble(road, "speed_limit_meters_per_second", context),
                .laneCount = requiredUnsigned<std::uint32_t>(road, "lane_count", context),
                .capacity = requiredUnsigned<std::uint32_t>(road, "capacity", context),
            },
        });
    }

    return network;
}

TrafficManager parseTrafficManager(const Json &document, const RoadNetwork &network)
{
    TrafficManager manager;
    const auto &controllers = requiredArray(document, "traffic_lights", "root");

    for (std::size_t controllerIndex = 0; controllerIndex < controllers.size(); ++controllerIndex)
    {
        const auto controllerContext = indexedContext("traffic_lights", controllerIndex);
        const auto &controllerValue = controllers[controllerIndex];

        if (!controllerValue.is_object())
        {
            throwInvalidScenario(controllerContext + " must be an object");
        }

        TrafficLightController controller{
            requiredUnsigned<IntersectionId>(controllerValue, "intersection_id", controllerContext),
        };

        const auto &lights = requiredArray(controllerValue, "lights", controllerContext);

        if (lights.empty())
        {
            throwInvalidScenario(controllerContext + ".lights must not be empty");
        }

        for (std::size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
        {
            const auto lightContext = indexedContext(controllerContext + ".lights", lightIndex);
            const auto &lightValue = lights[lightIndex];

            if (!lightValue.is_object())
            {
                throwInvalidScenario(lightContext + " must be an object");
            }

            const auto &timings = requiredObject(lightValue, "timings", lightContext);
            const auto initialStateText = requiredString(lightValue, "initial_state", lightContext);

            controller.addLight(
                network, requiredUnsigned<RoadId>(lightValue, "incoming_road_id", lightContext),
                TrafficLight{
                    TrafficLightTimings{
                        .greenSeconds =
                            requiredDouble(timings, "green_seconds", lightContext + ".timings"),
                        .yellowSeconds =
                            requiredDouble(timings, "yellow_seconds", lightContext + ".timings"),
                        .redSeconds =
                            requiredDouble(timings, "red_seconds", lightContext + ".timings"),
                    },
                    parseTrafficLightState(initialStateText,
                                           qualifiedName(lightContext, "initial_state")),
                });
        }

        manager.addController(network, std::move(controller));
    }

    return manager;
}

std::vector<VehicleSpawnRequest>
parseSpawnSchedule(const Json &document, const RoadNetwork &network, const SimulationConfig &config)
{
    const auto &spawnValues = requiredArray(document, "vehicle_spawns", "root");

    std::vector<VehicleSpawnRequest> schedule;
    schedule.reserve(spawnValues.size());

    const DijkstraRoutePlanner routePlanner;

    for (std::size_t index = 0; index < spawnValues.size(); ++index)
    {
        const auto context = indexedContext("vehicle_spawns", index);
        const auto &spawnValue = spawnValues[index];

        if (!spawnValue.is_object())
        {
            throwInvalidScenario(context + " must be an object");
        }

        const VehicleSpawnRequest request{
            .spawnTimeSeconds = requiredDouble(spawnValue, "spawn_time_seconds", context),
            .origin = requiredUnsigned<IntersectionId>(spawnValue, "origin", context),
            .destination = requiredUnsigned<IntersectionId>(spawnValue, "destination", context),
        };

        if (!std::isfinite(request.spawnTimeSeconds) || request.spawnTimeSeconds < 0.0)
        {
            throwInvalidScenario(qualifiedName(context, "spawn_time_seconds") +
                                 " must be finite and non-negative");
        }

        if (request.spawnTimeSeconds > config.durationSeconds)
        {
            throwInvalidScenario(qualifiedName(context, "spawn_time_seconds") +
                                 " cannot exceed the simulation duration");
        }

        if (!network.hasIntersection(request.origin))
        {
            throwInvalidScenario(qualifiedName(context, "origin") +
                                 " references an unknown intersection");
        }

        if (!network.hasIntersection(request.destination))
        {
            throwInvalidScenario(qualifiedName(context, "destination") +
                                 " references an unknown intersection");
        }

        if (!routePlanner.calculateRoute(network, request.origin, request.destination).has_value())
        {
            throwInvalidScenario(context + " has no route from origin to destination");
        }

        schedule.push_back(request);
    }

    return schedule;
}

SimulationConfig parseSimulationConfig(const Json &document)
{
    const auto &simulation = requiredObject(document, "simulation", "root");
    const auto &dynamics = requiredObject(simulation, "default_vehicle_dynamics", "simulation");

    SimulationConfig config{
        .durationSeconds = requiredDouble(simulation, "duration_seconds", "simulation"),
        .timeStepSeconds = requiredDouble(simulation, "time_step_seconds", "simulation"),
        .randomSeed = requiredUnsigned<std::uint64_t>(simulation, "random_seed", "simulation"),
        .maximumVehicles =
            requiredUnsigned<std::size_t>(simulation, "maximum_vehicles", "simulation"),
        .defaultVehicleDynamics =
            VehicleDynamics{
                .maximumSpeedMetersPerSecond =
                    requiredDouble(dynamics, "maximum_speed_meters_per_second",
                                   "simulation.default_vehicle_dynamics"),
                .accelerationMetersPerSecondSquared =
                    requiredDouble(dynamics, "acceleration_meters_per_second_squared",
                                   "simulation.default_vehicle_dynamics"),
                .decelerationMetersPerSecondSquared =
                    requiredDouble(dynamics, "deceleration_meters_per_second_squared",
                                   "simulation.default_vehicle_dynamics"),
            },
        .minimumFollowingDistanceMeters =
            requiredDouble(simulation, "minimum_following_distance_meters", "simulation"),
        .reactionTimeSeconds = requiredDouble(simulation, "reaction_time_seconds", "simulation"),
    };

    if (const auto *dynamicRouting = optionalObject(simulation, "dynamic_routing", "simulation");
        dynamicRouting != nullptr)
    {
        constexpr std::string_view context{"simulation.dynamic_routing"};

        config.rerouting = ReroutingConfig{
            .evaluationIntervalSeconds =
                requiredDouble(*dynamicRouting, "evaluation_interval_seconds", context),
            .minimumImprovementRatio =
                requiredDouble(*dynamicRouting, "minimum_improvement_ratio", context),
            .severeCongestionThreshold = parseCongestionState(
                requiredString(*dynamicRouting, "severe_congestion_threshold", context),
                qualifiedName(context, "severe_congestion_threshold")),
        };

        config.congestionCost = CongestionCostConfig{
            .minimumSpeedRatio = requiredDouble(*dynamicRouting, "minimum_speed_ratio", context),
            .moderatePenaltyMultiplier =
                requiredDouble(*dynamicRouting, "moderate_penalty_multiplier", context),
            .congestedPenaltyMultiplier =
                requiredDouble(*dynamicRouting, "congested_penalty_multiplier", context),
            .gridlockPenaltyMultiplier =
                requiredDouble(*dynamicRouting, "gridlock_penalty_multiplier", context),
        };
    }

    config.validate();
    return config;
}

} // namespace

Scenario ScenarioLoader::loadFromFile(const std::filesystem::path &filePath)
{
    if (filePath.empty())
    {
        throw std::invalid_argument{"Scenario file path must not be empty"};
    }

    std::ifstream input{filePath, std::ios::binary};

    if (!input.is_open())
    {
        throw std::runtime_error{"Could not open scenario file: " + filePath.string()};
    }

    const std::string jsonText{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };

    if (!input.good() && !input.eof())
    {
        throw std::runtime_error{"Could not read scenario file: " + filePath.string()};
    }

    return loadFromJson(jsonText);
}

Scenario ScenarioLoader::loadFromJson(std::string_view jsonText)
{
    if (jsonText.empty())
    {
        throw std::invalid_argument{"Scenario JSON text must not be empty"};
    }

    try
    {
        const auto document = Json::parse(jsonText.begin(), jsonText.end());

        if (!document.is_object())
        {
            throwInvalidScenario("root must be an object");
        }

        Scenario scenario;
        scenario.config = parseSimulationConfig(document);
        scenario.roadNetwork = parseRoadNetwork(document);
        scenario.trafficManager = parseTrafficManager(document, scenario.roadNetwork);
        scenario.spawnSchedule =
            parseSpawnSchedule(document, scenario.roadNetwork, scenario.config);
        return scenario;
    }
    catch (const nlohmann::json::exception &exception)
    {
        throw std::invalid_argument{"Invalid scenario JSON: " + std::string{exception.what()}};
    }
}

} // namespace trafficsim