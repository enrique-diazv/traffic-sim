#include "trafficsim/traffic/TrafficLight.h"

#include <cmath>
#include <exception>
#include <stdexcept>

namespace trafficsim
{

void TrafficLightTimings::validate() const
{
    if (!std::isfinite(greenSeconds) || greenSeconds <= 0.0)
    {
        throw std::invalid_argument{"Traffic light green duration must be finite and positive"};
    }

    if (!std::isfinite(yellowSeconds) || yellowSeconds <= 0.0)
    {
        throw std::invalid_argument{"Traffic light yellow duration must be finite and positive"};
    }

    if (!std::isfinite(redSeconds) || redSeconds <= 0.0)
    {
        throw std::invalid_argument{"Traffic light red duration must be finite and positive"};
    }

    if (!std::isfinite(greenSeconds + yellowSeconds + redSeconds))
    {
        throw std::invalid_argument{"Traffic light cycle duration must be finite"};
    }
}

TrafficLight::TrafficLight(TrafficLightTimings timings, TrafficLightState initialState)
    : timings_{timings}, initialState_{initialState}, state_{initialState}
{
    timings_.validate();

    switch (initialState_)
    {
    case TrafficLightState::Green:
    case TrafficLightState::Yellow:
    case TrafficLightState::Red:
        break;
    default:
        throw std::invalid_argument{"Traffic light initial state is invalid"};
    }
}

void TrafficLight::update(double deltaSeconds)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
    {
        throw std::invalid_argument{
            "Traffic light update duration must be finite and non-negative"};
    }

    const auto cycleDuration = timings_.greenSeconds + timings_.yellowSeconds + timings_.redSeconds;

    auto remainingUpdate = std::fmod(deltaSeconds, cycleDuration);

    while (remainingUpdate > 0.0)
    {
        const auto remainingState = stateDurationSeconds() - elapsedInStateSeconds_;

        if (remainingUpdate < remainingState)
        {
            elapsedInStateSeconds_ += remainingUpdate;
            break;
        }

        remainingUpdate -= remainingState;
        transitionToNextState();
    }
}

void TrafficLight::reset() noexcept
{
    state_ = initialState_;
    elapsedInStateSeconds_ = 0.0;
}

TrafficLightState TrafficLight::state() const noexcept
{
    return state_;
}

double TrafficLight::elapsedInStateSeconds() const noexcept
{
    return elapsedInStateSeconds_;
}

double TrafficLight::remainingInStateSeconds() const noexcept
{
    return stateDurationSeconds() - elapsedInStateSeconds_;
}

const TrafficLightTimings &TrafficLight::timings() const noexcept
{
    return timings_;
}

double TrafficLight::stateDurationSeconds() const noexcept
{
    switch (state_)
    {
    case TrafficLightState::Green:
        return timings_.greenSeconds;
    case TrafficLightState::Yellow:
        return timings_.yellowSeconds;
    case TrafficLightState::Red:
        return timings_.redSeconds;
    default:
        std::terminate();
    }
}

void TrafficLight::transitionToNextState() noexcept
{
    switch (state_)
    {
    case TrafficLightState::Green:
        state_ = TrafficLightState::Yellow;
        break;
    case TrafficLightState::Yellow:
        state_ = TrafficLightState::Red;
        break;
    case TrafficLightState::Red:
        state_ = TrafficLightState::Green;
        break;
    default:
        std::terminate();
    }

    elapsedInStateSeconds_ = 0.0;
}

} // namespace trafficsim
