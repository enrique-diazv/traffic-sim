#include "trafficsim/traffic/CongestionClassifier.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::CongestionClassifier;
using trafficsim::CongestionState;
using trafficsim::Road;
using trafficsim::RoadProperties;
using trafficsim::RoadTrafficObservation;

Road createTestRoad()
{
    return Road{
        10,
        RoadProperties{
            .origin = 1,
            .destination = 2,
            .lengthMeters = 1000.0,
            .speedLimitMetersPerSecond = 20.0,
            .laneCount = 1,
            .capacity = 100,
        },
    };
}

TEST(CongestionClassifierTests, TreatsEmptyRoadAsFreeFlow)
{
    const auto road = createTestRoad();

    const auto metrics = CongestionClassifier::evaluate(road, {});

    EXPECT_EQ(metrics.roadId, 10U);
    EXPECT_EQ(metrics.vehicleCount, 0U);
    EXPECT_DOUBLE_EQ(metrics.vehiclesPerKilometer, 0.0);
    EXPECT_DOUBLE_EQ(metrics.averageSpeedMetersPerSecond, 0.0);
    EXPECT_DOUBLE_EQ(metrics.occupancy, 0.0);
    EXPECT_DOUBLE_EQ(metrics.speedRatio, 0.0);
    EXPECT_EQ(metrics.congestionState, CongestionState::FreeFlow);
}

TEST(CongestionClassifierTests, CalculatesFreeFlowMetrics)
{
    const auto road = createTestRoad();

    const auto metrics =
        CongestionClassifier::evaluate(road, RoadTrafficObservation{
                                                 .vehicleCount = 20,
                                                 .totalSpeedMetersPerSecond = 300.0,
                                             });

    EXPECT_EQ(metrics.vehicleCount, 20U);
    EXPECT_DOUBLE_EQ(metrics.vehiclesPerKilometer, 20.0);
    EXPECT_DOUBLE_EQ(metrics.averageSpeedMetersPerSecond, 15.0);
    EXPECT_DOUBLE_EQ(metrics.occupancy, 0.2);
    EXPECT_DOUBLE_EQ(metrics.speedRatio, 0.75);
    EXPECT_EQ(metrics.congestionState, CongestionState::FreeFlow);
}

TEST(CongestionClassifierTests, ClassifiesIncreasingCongestion)
{
    const auto road = createTestRoad();

    const auto moderate =
        CongestionClassifier::evaluate(road, RoadTrafficObservation{
                                                 .vehicleCount = 30,
                                                 .totalSpeedMetersPerSecond = 450.0,
                                             });

    const auto congested =
        CongestionClassifier::evaluate(road, RoadTrafficObservation{
                                                 .vehicleCount = 60,
                                                 .totalSpeedMetersPerSecond = 240.0,
                                             });

    const auto gridlock =
        CongestionClassifier::evaluate(road, RoadTrafficObservation{
                                                 .vehicleCount = 80,
                                                 .totalSpeedMetersPerSecond = 80.0,
                                             });

    const auto fullCapacity =
        CongestionClassifier::evaluate(road, RoadTrafficObservation{
                                                 .vehicleCount = 100,
                                                 .totalSpeedMetersPerSecond = 2000.0,
                                             });

    EXPECT_EQ(moderate.congestionState, CongestionState::Moderate);
    EXPECT_EQ(congested.congestionState, CongestionState::Congested);
    EXPECT_EQ(gridlock.congestionState, CongestionState::Gridlock);
    EXPECT_EQ(fullCapacity.congestionState, CongestionState::Gridlock);
}

TEST(CongestionClassifierTests, RejectsInvalidObservations)
{
    const auto road = createTestRoad();

    EXPECT_THROW(
        static_cast<void>(CongestionClassifier::evaluate(road,
                                                         RoadTrafficObservation{
                                                             .vehicleCount = 1,
                                                             .totalSpeedMetersPerSecond = -1.0,
                                                         })),
        std::invalid_argument);

    EXPECT_THROW(static_cast<void>(CongestionClassifier::evaluate(
                     road,
                     RoadTrafficObservation{
                         .vehicleCount = 1,
                         .totalSpeedMetersPerSecond = std::numeric_limits<double>::infinity(),
                     })),
                 std::invalid_argument);

    EXPECT_THROW(
        static_cast<void>(CongestionClassifier::evaluate(road,
                                                         RoadTrafficObservation{
                                                             .vehicleCount = 0,
                                                             .totalSpeedMetersPerSecond = 1.0,
                                                         })),
        std::invalid_argument);
}

} // namespace