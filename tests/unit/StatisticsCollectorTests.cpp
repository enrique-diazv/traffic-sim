#include "trafficsim/statistics/StatisticsCollector.h"

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
using trafficsim::RoadTrafficMetrics;
using trafficsim::Route;
using trafficsim::StatisticsCollector;
using trafficsim::Vehicle;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleFollowingConstraint;
using trafficsim::VehicleState;

constexpr VehicleDynamics testDynamics()
{
    return VehicleDynamics{
        .maximumSpeedMetersPerSecond = 10.0,
        .accelerationMetersPerSecondSquared = 10.0,
        .decelerationMetersPerSecondSquared = 10.0,
    };
}

RoadNetwork createNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {0.0, 0.0}});
    network.addIntersection(Intersection{2, {10.0, 0.0}});

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 10.0,
            .speedLimitMetersPerSecond = 10.0,
            .laneCount = 1,
            .capacity = 2,
        },
    });

    return network;
}

TEST(StatisticsCollectorTests, StartsWithEmptyStatistics)
{
    const StatisticsCollector collector;
    const auto summary = collector.summary();

    EXPECT_EQ(summary.vehiclesSpawned, 0U);
    EXPECT_EQ(summary.vehiclesArrived, 0U);
    EXPECT_DOUBLE_EQ(summary.averageTravelTimeSeconds, 0.0);
    EXPECT_DOUBLE_EQ(summary.minimumTravelTimeSeconds, 0.0);
    EXPECT_DOUBLE_EQ(summary.maximumTravelTimeSeconds, 0.0);
    EXPECT_DOUBLE_EQ(summary.averageWaitingTimeSeconds, 0.0);
    EXPECT_DOUBLE_EQ(summary.averageSpeedMetersPerSecond, 0.0);
    EXPECT_DOUBLE_EQ(summary.totalDistanceMeters, 0.0);
    EXPECT_DOUBLE_EQ(summary.averageRouteLengthMeters, 0.0);
    EXPECT_EQ(summary.peakActiveVehicles, 0U);
    EXPECT_TRUE(collector.vehicleResults().empty());
}

TEST(StatisticsCollectorTests, AggregatesCompletedVehicleStatistics)
{
    const auto network = createNetwork();

    Vehicle firstVehicle{
        100, 1, 2, Route{{10}, 10.0}, testDynamics(),
    };
    ASSERT_TRUE(firstVehicle.start(network, 2.0));
    firstVehicle.update(1.0, network);
    ASSERT_EQ(firstVehicle.state(), VehicleState::Arrived);

    Vehicle secondVehicle{
        200, 1, 2, Route{{10}, 10.0}, testDynamics(),
    };
    ASSERT_TRUE(secondVehicle.start(network, 5.0));

    const VehicleFollowingConstraint blocked{
        .maximumPositionMeters = 5.0,
        .desiredSpeedLimitMetersPerSecond = 10.0,
    };

    secondVehicle.update(1.0, network, nullptr, &blocked);
    secondVehicle.update(2.0, network, nullptr, &blocked);

    const VehicleFollowingConstraint released{
        .maximumPositionMeters = 20.0,
        .desiredSpeedLimitMetersPerSecond = 10.0,
    };

    secondVehicle.update(1.0, network, nullptr, &released);
    ASSERT_EQ(secondVehicle.state(), VehicleState::Arrived);
    ASSERT_DOUBLE_EQ(secondVehicle.waitingTimeSeconds(), 2.0);

    std::array<Vehicle, 2> completedVehicles{
        std::move(firstVehicle),
        std::move(secondVehicle),
    };

    StatisticsCollector collector;
    collector.recordSpawned(completedVehicles.size());
    collector.observeActiveVehicles(completedVehicles);

    for (const auto &vehicle : completedVehicles)
    {
        collector.recordCompletedVehicle(vehicle);
    }

    const auto results = collector.vehicleResults();
    ASSERT_EQ(results.size(), 2U);

    EXPECT_EQ(results[0].vehicleId, 100U);
    EXPECT_DOUBLE_EQ(results[0].spawnTimeSeconds, 2.0);
    EXPECT_DOUBLE_EQ(results[0].arrivalTimeSeconds, 3.0);
    EXPECT_DOUBLE_EQ(results[0].travelTimeSeconds, 1.0);
    EXPECT_DOUBLE_EQ(results[0].waitingTimeSeconds, 0.0);
    EXPECT_DOUBLE_EQ(results[0].distanceMeters, 10.0);
    EXPECT_DOUBLE_EQ(results[0].averageSpeedMetersPerSecond, 10.0);

    EXPECT_EQ(results[1].vehicleId, 200U);
    EXPECT_DOUBLE_EQ(results[1].travelTimeSeconds, 4.0);
    EXPECT_DOUBLE_EQ(results[1].waitingTimeSeconds, 2.0);
    EXPECT_DOUBLE_EQ(results[1].averageSpeedMetersPerSecond, 2.5);

    const auto summary = collector.summary();

    EXPECT_EQ(summary.vehiclesSpawned, 2U);
    EXPECT_EQ(summary.vehiclesArrived, 2U);
    EXPECT_DOUBLE_EQ(summary.averageTravelTimeSeconds, 2.5);
    EXPECT_DOUBLE_EQ(summary.minimumTravelTimeSeconds, 1.0);
    EXPECT_DOUBLE_EQ(summary.maximumTravelTimeSeconds, 4.0);
    EXPECT_DOUBLE_EQ(summary.averageWaitingTimeSeconds, 1.0);
    EXPECT_DOUBLE_EQ(summary.averageSpeedMetersPerSecond, 4.0);
    EXPECT_DOUBLE_EQ(summary.totalDistanceMeters, 20.0);
    EXPECT_DOUBLE_EQ(summary.averageRouteLengthMeters, 10.0);
    EXPECT_EQ(summary.peakActiveVehicles, 2U);
}

TEST(StatisticsCollectorTests, AggregatesRoadStatistics)
{
    const std::array<RoadTrafficMetrics, 1> firstObservation{
        RoadTrafficMetrics{
            .roadId = 10,
            .vehicleCount = 2,
            .vehiclesPerKilometer = 200.0,
            .averageSpeedMetersPerSecond = 5.0,
            .occupancy = 1.0,
            .speedRatio = 0.5,
            .congestionState = CongestionState::Gridlock,
        },
    };

    const std::array<RoadTrafficMetrics, 1> secondObservation{
        RoadTrafficMetrics{
            .roadId = 10,
            .vehicleCount = 1,
            .vehiclesPerKilometer = 100.0,
            .averageSpeedMetersPerSecond = 0.0,
            .occupancy = 0.5,
            .speedRatio = 0.0,
            .congestionState = CongestionState::Moderate,
        },
    };

    StatisticsCollector collector;

    collector.observeRoads(1.0, firstObservation);
    collector.observeRoads(1.0, secondObservation);

    const auto roadResults = collector.roadResults();

    ASSERT_EQ(roadResults.size(), 1U);
    EXPECT_EQ(roadResults[0].roadId, 10U);
    EXPECT_NEAR(roadResults[0].averageSpeedMetersPerSecond, 10.0 / 3.0, 1.0e-9);
    EXPECT_EQ(roadResults[0].peakVehicleCount, 2U);
    EXPECT_DOUBLE_EQ(roadResults[0].averageOccupancy, 0.75);
    EXPECT_DOUBLE_EQ(roadResults[0].congestionTimeSeconds, 1.0);
    EXPECT_EQ(roadResults[0].peakCongestionState, CongestionState::Gridlock);
}

TEST(StatisticsCollectorTests, RejectsIncompleteAndDuplicateVehicles)
{
    const auto network = createNetwork();
    Vehicle vehicle{
        100, 1, 2, Route{{10}, 10.0}, testDynamics(),
    };

    StatisticsCollector collector;

    EXPECT_THROW(collector.recordCompletedVehicle(vehicle), std::invalid_argument);

    ASSERT_TRUE(vehicle.start(network));
    vehicle.update(1.0, network);
    ASSERT_EQ(vehicle.state(), VehicleState::Arrived);

    collector.recordCompletedVehicle(vehicle);

    EXPECT_THROW(collector.recordCompletedVehicle(vehicle), std::invalid_argument);
    EXPECT_EQ(collector.vehicleResults().size(), 1U);
}

} // namespace