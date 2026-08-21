#include "trafficsim/traffic/RoadTrafficMonitor.h"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <utility>

namespace
{

using trafficsim::CongestionState;
using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::RoadTrafficMonitor;
using trafficsim::Route;
using trafficsim::Vehicle;
using trafficsim::VehicleDynamics;

RoadNetwork createMonitorNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {.x = 0.0, .y = 0.0}});
    network.addIntersection(Intersection{2, {.x = 1000.0, .y = 0.0}});
    network.addIntersection(Intersection{3, {.x = 3000.0, .y = 0.0}});

    network.addRoad(Road{
        20,
        RoadProperties{
            .origin = 2,
            .destination = 3,
            .lengthMeters = 2000.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 4,
        },
    });

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 1000.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 4,
        },
    });

    return network;
}

Vehicle createMonitoredVehicle(const RoadNetwork &network)
{
    Vehicle vehicle{
        100,
        1,
        2,
        Route{{10}, 1000.0},
        VehicleDynamics{
            .maximumSpeedMetersPerSecond = 20.0,
            .accelerationMetersPerSecondSquared = 20.0,
            .decelerationMetersPerSecondSquared = 20.0,
        },
    };

    if (!vehicle.start(network))
    {
        throw std::logic_error{"Test vehicle could not start"};
    }

    vehicle.update(0.5, network);
    return vehicle;
}

TEST(RoadTrafficMonitorTests, BuildsMetricsForEveryRoad)
{
    const auto network = createMonitorNetwork();
    auto vehicle = createMonitoredVehicle(network);

    std::array<Vehicle, 1> vehicles{
        std::move(vehicle),
    };

    RoadTrafficMonitor monitor;
    monitor.update(network, vehicles);

    ASSERT_EQ(monitor.roadCount(), 2U);

    const auto &occupiedRoad = monitor.metricsFor(10);
    EXPECT_EQ(occupiedRoad.vehicleCount, 1U);
    EXPECT_DOUBLE_EQ(occupiedRoad.vehiclesPerKilometer, 1.0);
    EXPECT_DOUBLE_EQ(occupiedRoad.averageSpeedMetersPerSecond, 10.0);
    EXPECT_DOUBLE_EQ(occupiedRoad.occupancy, 0.25);
    EXPECT_DOUBLE_EQ(occupiedRoad.speedRatio, 0.5);
    EXPECT_EQ(occupiedRoad.congestionState, CongestionState::Moderate);

    const auto &emptyRoad = monitor.metricsFor(20);
    EXPECT_EQ(emptyRoad.vehicleCount, 0U);
    EXPECT_DOUBLE_EQ(emptyRoad.occupancy, 0.0);
    EXPECT_EQ(emptyRoad.congestionState, CongestionState::FreeFlow);
}

TEST(RoadTrafficMonitorTests, ReturnsMetricsInRoadIdentifierOrder)
{
    const auto network = createMonitorNetwork();

    RoadTrafficMonitor monitor;
    monitor.update(network, {});

    const auto metrics = monitor.allMetrics();

    ASSERT_EQ(metrics.size(), 2U);
    EXPECT_EQ(metrics[0].roadId, 10U);
    EXPECT_EQ(metrics[1].roadId, 20U);
}

TEST(RoadTrafficMonitorTests, ReplacesAndResetsSnapshot)
{
    const auto network = createMonitorNetwork();
    auto vehicle = createMonitoredVehicle(network);

    std::array<Vehicle, 1> vehicles{
        std::move(vehicle),
    };

    RoadTrafficMonitor monitor;
    monitor.update(network, vehicles);
    ASSERT_EQ(monitor.metricsFor(10).vehicleCount, 1U);

    monitor.update(network, {});
    EXPECT_EQ(monitor.metricsFor(10).vehicleCount, 0U);
    EXPECT_EQ(monitor.metricsFor(10).congestionState, CongestionState::FreeFlow);

    monitor.reset();

    EXPECT_EQ(monitor.roadCount(), 0U);
    EXPECT_FALSE(monitor.hasMetrics(10));
    EXPECT_THROW(static_cast<void>(monitor.metricsFor(10)), std::out_of_range);
}

} // namespace