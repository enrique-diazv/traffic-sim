#include "BenchmarkFixtures.h"

#include "trafficsim/network/Intersection.h"
#include "trafficsim/network/Road.h"
#include "trafficsim/routing/Route.h"
#include "trafficsim/vehicles/Vehicle.h"
#include "trafficsim/vehicles/VehicleSpawner.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace trafficsim::benchmarking
{

RoadNetwork createLinearNetwork(std::size_t roadCount, double roadLengthMeters)
{
    if (roadCount == 0U)
    {
        throw std::invalid_argument{"Benchmark network requires at least one road"};
    }

    if (!std::isfinite(roadLengthMeters) || roadLengthMeters <= 0.0)
    {
        throw std::invalid_argument{"Benchmark road length must be positive and finite"};
    }

    if (roadCount > std::numeric_limits<RoadId>::max() ||
        roadCount >= std::numeric_limits<IntersectionId>::max())
    {
        throw std::invalid_argument{"Benchmark network exceeds identifier capacity"};
    }

    RoadNetwork network;

    for (std::size_t index = 0U; index <= roadCount; ++index)
    {
        const auto intersectionId = static_cast<IntersectionId>(index + 1U);

        network.addIntersection(Intersection{
            intersectionId,
            Position{
                .x = static_cast<double>(index) * roadLengthMeters,
                .y = 0.0,
            },
        });
    }

    for (std::size_t index = 0U; index < roadCount; ++index)
    {
        const auto roadId = static_cast<RoadId>(index + 1U);
        const auto origin = static_cast<IntersectionId>(index + 1U);
        const auto destination = static_cast<IntersectionId>(index + 2U);

        network.addRoad(Road{
            roadId,
            RoadProperties{
                .origin = origin,
                .destination = destination,
                .lengthMeters = roadLengthMeters,
                .speedLimitMetersPerSecond = 20.0,
                .laneCount = 1U,
                .capacity = 10'000U,
            },
        });
    }

    return network;
}

VehicleManager createVehicleManager(std::size_t vehicleCount, const RoadNetwork &network)
{
    if (vehicleCount == 0U)
    {
        throw std::invalid_argument{"Benchmark vehicle count must be positive"};
    }

    VehicleManager manager{vehicleCount};

    const VehicleDynamics dynamics{
        .maximumSpeedMetersPerSecond = 20.0,
        .accelerationMetersPerSecondSquared = 4.0,
        .decelerationMetersPerSecondSquared = 6.0,
    };

    const auto roadLength = network.getRoad(1U).lengthMeters();

    for (std::size_t index = 0U; index < vehicleCount; ++index)
    {
        Vehicle vehicle{
            static_cast<VehicleId>(index + 1U), 1U, 2U, Route{{1U}, roadLength}, dynamics,
        };

        if (!vehicle.start(network))
        {
            throw std::logic_error{"Benchmark vehicle could not start"};
        }

        manager.addVehicle(std::move(vehicle));
    }

    return manager;
}

std::vector<RoadTrafficMetrics> createRoadMetrics(std::size_t metricCount)
{
    if (metricCount == 0U || metricCount > std::numeric_limits<RoadId>::max())
    {
        throw std::invalid_argument{"Benchmark road metric count is invalid"};
    }

    std::vector<RoadTrafficMetrics> metrics;
    metrics.reserve(metricCount);

    for (std::size_t index = 0U; index < metricCount; ++index)
    {
        metrics.push_back(RoadTrafficMetrics{
            .roadId = static_cast<RoadId>(index + 1U),
            .vehicleCount = 1U,
            .vehiclesPerKilometer = 10.0,
            .averageSpeedMetersPerSecond = 10.0,
            .occupancy = 0.25,
            .speedRatio = 0.5,
            .congestionState = CongestionState::Moderate,
        });
    }

    return metrics;
}

Simulation createSimulation(std::size_t vehicleCount, double timeStepSeconds,
                            double durationSeconds)
{
    if (vehicleCount == 0U)
    {
        throw std::invalid_argument{"Benchmark simulation requires vehicles"};
    }

    SimulationConfig config;
    config.durationSeconds = durationSeconds;
    config.timeStepSeconds = timeStepSeconds;
    config.maximumVehicles = vehicleCount;
    config.defaultVehicleDynamics = VehicleDynamics{
        .maximumSpeedMetersPerSecond = 20.0,
        .accelerationMetersPerSecondSquared = 4.0,
        .decelerationMetersPerSecondSquared = 6.0,
    };

    auto network = createLinearNetwork(1U, 1'000'000.0);

    std::vector<VehicleSpawnRequest> schedule;
    schedule.reserve(vehicleCount);

    for (std::size_t index = 0U; index < vehicleCount; ++index)
    {
        static_cast<void>(index);

        schedule.push_back(VehicleSpawnRequest{
            .spawnTimeSeconds = 0.0,
            .origin = 1U,
            .destination = 2U,
        });
    }

    return Simulation{
        config,
        std::move(network),
        std::move(schedule),
    };
}

} // namespace trafficsim::benchmarking