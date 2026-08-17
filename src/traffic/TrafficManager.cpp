#include "trafficsim/traffic/TrafficManager.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace trafficsim
{

void TrafficManager::addController(const RoadNetwork &network, TrafficLightController controller)
{
    const auto intersectionId = controller.intersectionId();

    if (!network.hasIntersection(intersectionId))
    {
        throw std::invalid_argument{"Traffic manager controller intersection does not exist"};
    }

    if (hasController(intersectionId))
    {
        throw std::invalid_argument{"Traffic manager already contains an intersection controller"};
    }

    controllers_.push_back(std::move(controller));
}

bool TrafficManager::hasController(IntersectionId intersectionId) const noexcept
{
    return std::ranges::any_of(controllers_,
                               [intersectionId](const TrafficLightController &controller)
                               { return controller.intersectionId() == intersectionId; });
}

const TrafficLightController &TrafficManager::getController(IntersectionId intersectionId) const
{
    const auto controller =
        std::ranges::find_if(controllers_, [intersectionId](const TrafficLightController &candidate)
                             { return candidate.intersectionId() == intersectionId; });

    if (controller == controllers_.end())
    {
        throw std::out_of_range{"Traffic manager controller was not found"};
    }

    return *controller;
}

std::optional<TrafficLightState> TrafficManager::stateForRoad(RoadId incomingRoadId) const
{
    for (const auto &controller : controllers_)
    {
        if (controller.hasLight(incomingRoadId))
        {
            return controller.stateForRoad(incomingRoadId);
        }
    }

    return std::nullopt;
}

bool TrafficManager::allowsEntry(RoadId incomingRoadId) const
{
    const auto state = stateForRoad(incomingRoadId);

    return !state.has_value() || *state == TrafficLightState::Green;
}

void TrafficManager::update(double deltaSeconds)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
    {
        throw std::invalid_argument{
            "Traffic manager update duration must be finite and non-negative"};
    }

    for (auto &controller : controllers_)
    {
        controller.update(deltaSeconds);
    }
}

void TrafficManager::reset() noexcept
{
    for (auto &controller : controllers_)
    {
        controller.reset();
    }
}

std::size_t TrafficManager::controllerCount() const noexcept
{
    return controllers_.size();
}

} // namespace trafficsim
