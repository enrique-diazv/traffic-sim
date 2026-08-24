#include "trafficsim/experiments/ExperimentAggregator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace trafficsim
{
namespace
{

class RunningMetric final
{
  public:
    void add(double value)
    {
        if (!std::isfinite(value))
        {
            throw std::invalid_argument{"Experiment metric values must be finite"};
        }

        ++count_;

        const auto delta = value - mean_;
        mean_ += delta / static_cast<double>(count_);
        const auto updatedDelta = value - mean_;
        squaredDifferenceTotal_ += delta * updatedDelta;

        minimum_ = std::min(minimum_, value);
        maximum_ = std::max(maximum_, value);
    }

    [[nodiscard]] AggregateMetric result() const
    {
        if (count_ == 0U)
        {
            throw std::logic_error{"Cannot aggregate an empty metric"};
        }

        const auto variance = std::max(0.0, squaredDifferenceTotal_ / static_cast<double>(count_));

        return {
            .mean = mean_,
            .minimum = minimum_,
            .maximum = maximum_,
            .standardDeviation = std::sqrt(variance),
        };
    }

  private:
    std::size_t count_{};
    double mean_{};
    double squaredDifferenceTotal_{};
    double minimum_{std::numeric_limits<double>::infinity()};
    double maximum_{-std::numeric_limits<double>::infinity()};
};

struct VariantAccumulator
{
    std::string variantName;
    std::size_t runCount{};

    RunningMetric vehiclesSpawned;
    RunningMetric vehiclesArrived;
    RunningMetric totalReroutes;
    RunningMetric averageTravelTimeSeconds;
    RunningMetric averageWaitingTimeSeconds;
    RunningMetric averageSpeedMetersPerSecond;
    RunningMetric totalDistanceMeters;
    RunningMetric averageRouteLengthMeters;
    RunningMetric peakActiveVehicles;

    void add(const ExperimentRunResult &runResult)
    {
        ++runCount;

        vehiclesSpawned.add(static_cast<double>(runResult.summary.vehiclesSpawned));
        vehiclesArrived.add(static_cast<double>(runResult.summary.vehiclesArrived));
        totalReroutes.add(static_cast<double>(runResult.totalReroutes));
        averageTravelTimeSeconds.add(runResult.summary.averageTravelTimeSeconds);
        averageWaitingTimeSeconds.add(runResult.summary.averageWaitingTimeSeconds);
        averageSpeedMetersPerSecond.add(runResult.summary.averageSpeedMetersPerSecond);
        totalDistanceMeters.add(runResult.summary.totalDistanceMeters);
        averageRouteLengthMeters.add(runResult.summary.averageRouteLengthMeters);
        peakActiveVehicles.add(static_cast<double>(runResult.summary.peakActiveVehicles));
    }

    [[nodiscard]] AggregatedExperimentResult result() const
    {
        return {
            .variantName = variantName,
            .runCount = runCount,
            .vehiclesSpawned = vehiclesSpawned.result(),
            .vehiclesArrived = vehiclesArrived.result(),
            .totalReroutes = totalReroutes.result(),
            .averageTravelTimeSeconds = averageTravelTimeSeconds.result(),
            .averageWaitingTimeSeconds = averageWaitingTimeSeconds.result(),
            .averageSpeedMetersPerSecond = averageSpeedMetersPerSecond.result(),
            .totalDistanceMeters = totalDistanceMeters.result(),
            .averageRouteLengthMeters = averageRouteLengthMeters.result(),
            .peakActiveVehicles = peakActiveVehicles.result(),
        };
    }
};

} // namespace

std::vector<AggregatedExperimentResult>
ExperimentAggregator::aggregate(std::span<const ExperimentRunResult> runResults)
{
    if (runResults.empty())
    {
        throw std::invalid_argument{"Experiment aggregation requires at least one run"};
    }

    std::unordered_map<std::string, std::size_t> variantIndexes;
    variantIndexes.reserve(runResults.size());

    std::vector<VariantAccumulator> accumulators;
    accumulators.reserve(runResults.size());

    for (const auto &runResult : runResults)
    {
        if (runResult.variantName.empty())
        {
            throw std::invalid_argument{"Experiment run variant name must not be empty"};
        }

        const auto [indexIterator, inserted] =
            variantIndexes.try_emplace(runResult.variantName, accumulators.size());

        if (inserted)
        {
            accumulators.push_back(VariantAccumulator{
                .variantName = runResult.variantName,
            });
        }

        accumulators[indexIterator->second].add(runResult);
    }

    std::vector<AggregatedExperimentResult> aggregatedResults;
    aggregatedResults.reserve(accumulators.size());

    for (const auto &accumulator : accumulators)
    {
        aggregatedResults.push_back(accumulator.result());
    }

    return aggregatedResults;
}

} // namespace trafficsim