#include "trafficsim/statistics/StatisticsCollector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace trafficsim
{

void StatisticsCollector::recordSpawned(std::size_t count)
{
    if (count > std::numeric_limits<std::size_t>::max() - vehiclesSpawned_)
    {
        throw std::overflow_error{"Spawned vehicle count overflow"};
    }

    vehiclesSpawned_ += count;
}

void StatisticsCollector::observeActiveVehicles(std::span<const Vehicle> vehicles) noexcept
{
    peakActiveVehicles_ = std::max(peakActiveVehicles_, vehicles.size());
}

void StatisticsCollector::observeRoads(double deltaSeconds,
                                       std::span<const RoadTrafficMetrics> roadMetrics)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
    {
        throw std::invalid_argument{"Road observation duration must be finite and non-negative"};
    }

    for (const auto &metrics : roadMetrics)
    {
        if (!std::isfinite(metrics.averageSpeedMetersPerSecond) ||
            metrics.averageSpeedMetersPerSecond < 0.0 || !std::isfinite(metrics.occupancy) ||
            metrics.occupancy < 0.0)
        {
            throw std::invalid_argument{"Road metrics must be finite and non-negative"};
        }

        auto [accumulatorIterator, inserted] = roadAccumulators_.try_emplace(metrics.roadId);
        auto &accumulator = accumulatorIterator->second;

        if (inserted)
        {
            accumulator.result.roadId = metrics.roadId;
        }

        accumulator.totalSpeedMetersPerSecond +=
            metrics.averageSpeedMetersPerSecond * static_cast<double>(metrics.vehicleCount);
        accumulator.speedObservationCount += metrics.vehicleCount;
        accumulator.totalOccupancy += metrics.occupancy;
        ++accumulator.stepObservationCount;

        accumulator.result.peakVehicleCount =
            std::max(accumulator.result.peakVehicleCount, metrics.vehicleCount);

        accumulator.result.peakCongestionState =
            std::max(accumulator.result.peakCongestionState, metrics.congestionState);

        if (metrics.congestionState == CongestionState::Congested ||
            metrics.congestionState == CongestionState::Gridlock)
        {
            accumulator.result.congestionTimeSeconds += deltaSeconds;
        }

        if (accumulator.speedObservationCount > 0)
        {
            accumulator.result.averageSpeedMetersPerSecond =
                accumulator.totalSpeedMetersPerSecond /
                static_cast<double>(accumulator.speedObservationCount);
        }

        accumulator.result.averageOccupancy =
            accumulator.totalOccupancy / static_cast<double>(accumulator.stepObservationCount);
    }
}

void StatisticsCollector::recordCompletedVehicle(const Vehicle &vehicle)
{
    if (vehicle.state() != VehicleState::Arrived)
    {
        throw std::invalid_argument{"Statistics require an arrived vehicle"};
    }

    const auto spawnTime = vehicle.spawnTimeSeconds();
    const auto arrivalTime = vehicle.arrivalTimeSeconds();
    const auto travelTime = vehicle.travelTimeSeconds();

    if (!spawnTime.has_value() || !arrivalTime.has_value() || !travelTime.has_value())
    {
        throw std::logic_error{"Arrived vehicle has incomplete timing data"};
    }

    if (completedVehicleIds_.contains(vehicle.id()))
    {
        throw std::invalid_argument{"Vehicle statistics were already recorded"};
    }

    const auto distance = vehicle.route().totalDistanceMeters();
    const auto averageSpeed = *travelTime > 0.0 ? distance / *travelTime : 0.0;

    vehicleResults_.push_back(VehicleResult{
        .vehicleId = vehicle.id(),
        .origin = vehicle.origin(),
        .destination = vehicle.destination(),
        .spawnTimeSeconds = *spawnTime,
        .arrivalTimeSeconds = *arrivalTime,
        .travelTimeSeconds = *travelTime,
        .waitingTimeSeconds = vehicle.waitingTimeSeconds(),
        .distanceMeters = distance,
        .averageSpeedMetersPerSecond = averageSpeed,
    });

    const auto [completedIterator, inserted] = completedVehicleIds_.insert(vehicle.id());
    static_cast<void>(completedIterator);

    if (!inserted)
    {
        vehicleResults_.pop_back();
        throw std::logic_error{"Completed vehicle identifier insertion failed"};
    }
}

void StatisticsCollector::reset() noexcept
{
    vehiclesSpawned_ = 0;
    peakActiveVehicles_ = 0;
    vehicleResults_.clear();
    completedVehicleIds_.clear();
    roadAccumulators_.clear();
}

SimulationSummary StatisticsCollector::summary() const noexcept
{
    SimulationSummary result{
        .vehiclesSpawned = vehiclesSpawned_,
        .vehiclesArrived = vehicleResults_.size(),
        .peakActiveVehicles = peakActiveVehicles_,
    };

    if (vehicleResults_.empty())
    {
        return result;
    }

    result.minimumTravelTimeSeconds = vehicleResults_.front().travelTimeSeconds;
    result.maximumTravelTimeSeconds = vehicleResults_.front().travelTimeSeconds;

    double totalTravelTime = 0.0;
    double totalWaitingTime = 0.0;

    for (const auto &vehicle : vehicleResults_)
    {
        totalTravelTime += vehicle.travelTimeSeconds;
        totalWaitingTime += vehicle.waitingTimeSeconds;
        result.totalDistanceMeters += vehicle.distanceMeters;
        result.minimumTravelTimeSeconds =
            std::min(result.minimumTravelTimeSeconds, vehicle.travelTimeSeconds);
        result.maximumTravelTimeSeconds =
            std::max(result.maximumTravelTimeSeconds, vehicle.travelTimeSeconds);
    }

    const auto arrivedCount = static_cast<double>(vehicleResults_.size());

    result.averageTravelTimeSeconds = totalTravelTime / arrivedCount;
    result.averageWaitingTimeSeconds = totalWaitingTime / arrivedCount;
    result.averageRouteLengthMeters = result.totalDistanceMeters / arrivedCount;

    if (totalTravelTime > 0.0)
    {
        result.averageSpeedMetersPerSecond = result.totalDistanceMeters / totalTravelTime;
    }

    return result;
}

std::span<const VehicleResult> StatisticsCollector::vehicleResults() const noexcept
{
    return vehicleResults_;
}

std::vector<RoadResult> StatisticsCollector::roadResults() const
{
    std::vector<RoadResult> results;
    results.reserve(roadAccumulators_.size());

    for (const auto &[roadId, accumulator] : roadAccumulators_)
    {
        static_cast<void>(roadId);
        results.push_back(accumulator.result);
    }

    std::ranges::sort(results, {}, &RoadResult::roadId);
    return results;
}

} // namespace trafficsim