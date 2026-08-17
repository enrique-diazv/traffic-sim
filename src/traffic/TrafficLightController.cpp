#include "trafficsim/traffic/TrafficLightController.h"

#include <cmath>
#include <stdexcept>

namespace trafficsim
{

TrafficLightController::TrafficLightController(IntersectionId intersectionId)
    : intersectionId_{intersectionId}
{
}

void TrafficLightController::addLight(const RoadNetwork &network, RoadId incomingRoadId,
                                      TrafficLight light)
{
    if (!network.hasIntersection(intersectionId_))
    {
        throw std::invalid_argument{"Traffic light controller intersection does not exist"};
    }

    if (!network.hasRoad(incomingRoadId))
    {
        throw std::invalid_argument{"Traffic light incoming road does not exist"};
    }

    if (network.getRoad(incomingRoadId).destination() != intersectionId_)
    {
        throw std::invalid_argument{
            "Traffic light road does not enter the controlled intersection"};
    }

    const auto [lightIterator, inserted] = lights_.emplace(incomingRoadId, light);

    static_cast<void>(lightIterator);

    if (!inserted)
    {
        throw std::invalid_argument{"Traffic light already exists for the incoming road"};
    }
}

bool TrafficLightController::hasLight(RoadId incomingRoadId) const noexcept
{
    return lights_.contains(incomingRoadId);
}

const TrafficLight &TrafficLightController::getLight(RoadId incomingRoadId) const
{
    const auto light = lights_.find(incomingRoadId);

    if (light == lights_.end())
    {
        throw std::out_of_range{"Traffic light was not found for the incoming road"};
    }

    return light->second;
}

TrafficLightState TrafficLightController::stateForRoad(RoadId incomingRoadId) const
{
    return getLight(incomingRoadId).state();
}

bool TrafficLightController::allowsEntry(RoadId incomingRoadId) const
{
    return stateForRoad(incomingRoadId) == TrafficLightState::Green;
}

void TrafficLightController::update(double deltaSeconds)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
    {
        throw std::invalid_argument{
            "Traffic light controller update duration must be finite and non-negative"};
    }

    for (auto &[roadId, light] : lights_)
    {
        static_cast<void>(roadId);
        light.update(deltaSeconds);
    }
}

void TrafficLightController::reset() noexcept
{
    for (auto &[roadId, light] : lights_)
    {
        static_cast<void>(roadId);
        light.reset();
    }
}

IntersectionId TrafficLightController::intersectionId() const noexcept
{
    return intersectionId_;
}

std::size_t TrafficLightController::lightCount() const noexcept
{
    return lights_.size();
}

} // namespace trafficsim
