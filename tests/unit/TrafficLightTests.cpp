#include "trafficsim/traffic/TrafficLight.h"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

namespace
{

using trafficsim::TrafficLight;
using trafficsim::TrafficLightState;
using trafficsim::TrafficLightTimings;

constexpr TrafficLightTimings testTimings()
{
    return TrafficLightTimings{
        .greenSeconds = 10.0,
        .yellowSeconds = 2.0,
        .redSeconds = 8.0,
    };
}

TEST(TrafficLightTests, StartsInConfiguredState)
{
    const TrafficLight light{testTimings()};

    EXPECT_EQ(light.state(), TrafficLightState::Green);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 0.0);
    EXPECT_DOUBLE_EQ(light.remainingInStateSeconds(), 10.0);
    EXPECT_DOUBLE_EQ(light.timings().yellowSeconds, 2.0);
}

TEST(TrafficLightTests, TransitionsThroughCompleteCycle)
{
    TrafficLight light{testTimings()};

    light.update(10.0);

    EXPECT_EQ(light.state(), TrafficLightState::Yellow);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 0.0);

    light.update(2.0);

    EXPECT_EQ(light.state(), TrafficLightState::Red);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 0.0);

    light.update(8.0);

    EXPECT_EQ(light.state(), TrafficLightState::Green);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 0.0);
}

TEST(TrafficLightTests, PreservesTimeAcrossTransitions)
{
    TrafficLight light{testTimings()};

    light.update(12.5);

    EXPECT_EQ(light.state(), TrafficLightState::Red);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 0.5);
    EXPECT_DOUBLE_EQ(light.remainingInStateSeconds(), 7.5);
}

TEST(TrafficLightTests, SkipsCompleteCyclesEfficiently)
{
    TrafficLight light{testTimings()};

    light.update(20'000'011.0);

    EXPECT_EQ(light.state(), TrafficLightState::Yellow);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 1.0);
    EXPECT_DOUBLE_EQ(light.remainingInStateSeconds(), 1.0);
}

TEST(TrafficLightTests, ResetRestoresInitialState)
{
    TrafficLight light{
        testTimings(),
        TrafficLightState::Red,
    };

    light.update(9.0);

    EXPECT_EQ(light.state(), TrafficLightState::Green);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 1.0);

    light.reset();

    EXPECT_EQ(light.state(), TrafficLightState::Red);
    EXPECT_DOUBLE_EQ(light.elapsedInStateSeconds(), 0.0);
    EXPECT_DOUBLE_EQ(light.remainingInStateSeconds(), 8.0);
}

TEST(TrafficLightTests, RejectsInvalidConfiguration)
{
    auto timings = testTimings();

    timings.greenSeconds = 0.0;
    EXPECT_THROW((TrafficLight{timings}), std::invalid_argument);

    timings = testTimings();
    timings.yellowSeconds = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW((TrafficLight{timings}), std::invalid_argument);

    timings = testTimings();
    timings.redSeconds = std::numeric_limits<double>::infinity();
    EXPECT_THROW((TrafficLight{timings}), std::invalid_argument);

    EXPECT_THROW((TrafficLight{
                     testTimings(),
                     static_cast<TrafficLightState>(255),
                 }),
                 std::invalid_argument);
}

TEST(TrafficLightTests, RejectsInvalidUpdateDuration)
{
    TrafficLight light{testTimings()};

    EXPECT_THROW(light.update(-0.1), std::invalid_argument);
    EXPECT_THROW(light.update(std::numeric_limits<double>::infinity()), std::invalid_argument);
    EXPECT_THROW(light.update(std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
}

} // namespace
