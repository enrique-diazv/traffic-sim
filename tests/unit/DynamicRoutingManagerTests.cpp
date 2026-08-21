#include "trafficsim/routing/DynamicRoutingManager.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace
{

using trafficsim::DynamicRoutingManager;
using trafficsim::Intersection;
using trafficsim::Road;
using trafficsim::RoadNetwork;
using trafficsim::RoadProperties;
using trafficsim::RoadTrafficMonitor;
using trafficsim::Route;
using trafficsim::Vehicle;
using trafficsim::VehicleDynamics;
using trafficsim::VehicleManager;

constexpr VehicleDynamics testDynamics()
{
    return VehicleDynamics{
        .maximumSpeedMetersPerSecond = 20.0,
        .accelerationMetersPerSecondSquared = 5.0,
        .decelerationMetersPerSecondSquared = 5.0,
    };
}

RoadNetwork createNetwork()
{
    RoadNetwork network;

    network.addIntersection(Intersection{1, {.x = 0.0, .y = 0.0}});
    network.addIntersection(Intersection{2, {.x = 100.0, .y = 0.0}});
    network.addIntersection(Intersection{3, {.x = 200.0, .y = 0.0}});
    network.addIntersection(Intersection{4, {.x = 100.0, .y = 100.0}});

    network.addRoad(Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 100.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 10,
        },
    });
    network.addRoad(Road{
        20,
        RoadProperties{
            .origin = 2,
            .destination = 3,
            .lengthMeters = 100.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 1,
        },
    });
    network.addRoad(Road{
        30,
        RoadProperties{
            .origin = 2,
            .destination = 4,
            .lengthMeters = 120.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 10,
        },
    });
    network.addRoad(Road{
        40,
        RoadProperties{
            .origin = 4,
            .destination = 3,
            .lengthMeters = 120.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 10,
        },
    });

    return network;
}

VehicleManager createVehicles(const RoadNetwork &network, bool includeBlocker)
{
    VehicleManager vehicles{10};

    Vehicle target{
        1, 1, 3, Route{{10, 20}, 200.0}, testDynamics(),
    };
    if (!target.start(network))
    {
        throw std::logic_error{"Test target vehicle could not start"};
    }
    vehicles.addVehicle(std::move(target));

    if (includeBlocker)
    {
        Vehicle blocker{
            2, 2, 3, Route{{20}, 100.0}, testDynamics(),
        };
        if (!blocker.start(network))
        {
            throw std::logic_error{"Test blocker vehicle could not start"};
        }
        vehicles.addVehicle(std::move(blocker));
    }

    return vehicles;
}

TEST(DynamicRoutingManagerTests, ReroutesAroundSevereCongestion)
{
    const auto network = createNetwork();
    auto vehicles = createVehicles(network, true);
    RoadTrafficMonitor trafficMonitor;
    DynamicRoutingManager routingManager;

    trafficMonitor.update(network, vehicles.vehicles());

    const auto result = routingManager.update(0.0, network, trafficMonitor, vehicles);

    EXPECT_EQ(result.evaluatedVehicles, 1U);
    EXPECT_EQ(result.reroutedVehicles, 1U);
    EXPECT_EQ(routingManager.totalEvaluations(), 1U);
    EXPECT_EQ(routingManager.totalReroutes(), 1U);

    const auto &target = vehicles.getVehicle(1);

    ASSERT_EQ(target.route().segments().size(), 3U);
    EXPECT_EQ(target.route().segments()[0], 10U);
    EXPECT_EQ(target.route().segments()[1], 30U);
    EXPECT_EQ(target.route().segments()[2], 40U);
    EXPECT_EQ(target.currentRoad(), 10U);
}

TEST(DynamicRoutingManagerTests, RespectsEvaluationInterval)
{
    const auto network = createNetwork();
    auto vehicles = createVehicles(network, true);
    RoadTrafficMonitor trafficMonitor;
    DynamicRoutingManager routingManager;

    trafficMonitor.update(network, vehicles.vehicles());

    const auto initial = routingManager.update(0.0, network, trafficMonitor, vehicles);
    const auto tooSoon = routingManager.update(4.9, network, trafficMonitor, vehicles);
    const auto due = routingManager.update(5.0, network, trafficMonitor, vehicles);

    EXPECT_EQ(initial.evaluatedVehicles, 1U);
    EXPECT_EQ(initial.reroutedVehicles, 1U);
    EXPECT_EQ(tooSoon.evaluatedVehicles, 0U);
    EXPECT_EQ(tooSoon.reroutedVehicles, 0U);
    EXPECT_EQ(due.evaluatedVehicles, 1U);
    EXPECT_EQ(due.reroutedVehicles, 0U);
    EXPECT_EQ(routingManager.totalEvaluations(), 2U);
    EXPECT_EQ(routingManager.totalReroutes(), 1U);
}

TEST(DynamicRoutingManagerTests, KeepsBestRouteAndResetsHistory)
{
    const auto network = createNetwork();
    auto vehicles = createVehicles(network, false);
    RoadTrafficMonitor trafficMonitor;
    DynamicRoutingManager routingManager;

    trafficMonitor.update(network, vehicles.vehicles());

    const auto result = routingManager.update(0.0, network, trafficMonitor, vehicles);

    EXPECT_EQ(result.evaluatedVehicles, 1U);
    EXPECT_EQ(result.reroutedVehicles, 0U);
    EXPECT_EQ(vehicles.getVehicle(1).route().segments()[1], 20U);

    routingManager.reset();

    EXPECT_EQ(routingManager.totalEvaluations(), 0U);
    EXPECT_EQ(routingManager.totalReroutes(), 0U);

    const auto afterReset = routingManager.update(1.0, network, trafficMonitor, vehicles);

    EXPECT_EQ(afterReset.evaluatedVehicles, 1U);
    EXPECT_EQ(afterReset.reroutedVehicles, 0U);
}

} // namespace