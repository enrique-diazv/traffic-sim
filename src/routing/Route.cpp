#include "trafficsim/routing/Route.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace trafficsim
{

Route::Route(std::vector<RoadId> roadIds, double totalDistanceMeters)
    : roadIds_{std::move(roadIds)}, totalDistanceMeters_{totalDistanceMeters}
{
    if (!std::isfinite(totalDistanceMeters_) || totalDistanceMeters_ < 0.0)
    {
        throw std::invalid_argument{"Route distance must be finite and non-negative"};
    }

    if (roadIds_.empty() && totalDistanceMeters_ != 0.0)
    {
        throw std::invalid_argument{"An empty route must have zero distance"};
    }

    if (!roadIds_.empty() && totalDistanceMeters_ == 0.0)
    {
        throw std::invalid_argument{"A non-empty route must have positive distance"};
    }
}

std::span<const RoadId> Route::segments() const noexcept
{
    return std::span<const RoadId>{roadIds_};
}

void Route::replaceRemainingSegments(std::vector<RoadId> roadIds, double totalDistanceMeters)
{
    if (isComplete())
    {
        throw std::logic_error{"Cannot replace segments of a completed route"};
    }

    if (roadIds.empty() || roadIds.front() != *currentRoad())
    {
        throw std::invalid_argument{"Replacement route must preserve the current road"};
    }

    if (!std::isfinite(totalDistanceMeters) || totalDistanceMeters <= 0.0)
    {
        throw std::invalid_argument{"Replacement route distance must be finite and positive"};
    }

    const auto completedSegmentCount = segmentCount() - remainingSegments().size();

    std::vector<RoadId> updatedRoadIds;
    updatedRoadIds.reserve(completedSegmentCount + roadIds.size());
    updatedRoadIds.insert(updatedRoadIds.end(), roadIds_.begin(),
                          roadIds_.begin() + static_cast<std::vector<RoadId>::difference_type>(
                                                 completedSegmentCount));
    updatedRoadIds.insert(updatedRoadIds.end(), roadIds.begin(), roadIds.end());

    roadIds_ = std::move(updatedRoadIds);
    totalDistanceMeters_ = totalDistanceMeters;
}

std::span<const RoadId> Route::remainingSegments() const noexcept
{
    return segments().subspan(currentSegment_);
}

std::optional<RoadId> Route::currentRoad() const noexcept
{
    if (isComplete())
    {
        return std::nullopt;
    }

    return roadIds_[currentSegment_];
}

std::optional<RoadId> Route::nextRoad() const noexcept
{
    const auto nextSegment = currentSegment_ + 1;

    if (isComplete() || nextSegment >= roadIds_.size())
    {
        return std::nullopt;
    }

    return roadIds_[nextSegment];
}

bool Route::advance() noexcept
{
    if (isComplete())
    {
        return false;
    }

    ++currentSegment_;
    return true;
}

bool Route::isComplete() const noexcept
{
    return currentSegment_ >= roadIds_.size();
}

std::size_t Route::segmentCount() const noexcept
{
    return roadIds_.size();
}

double Route::totalDistanceMeters() const noexcept
{
    return totalDistanceMeters_;
}

} // namespace trafficsim