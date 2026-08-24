#ifndef TRAFFICSIM_EXPERIMENTS_EXPERIMENT_AGGREGATOR_H
#define TRAFFICSIM_EXPERIMENTS_EXPERIMENT_AGGREGATOR_H

#include "trafficsim/experiments/BatchExperimentRunner.h"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace trafficsim
{

struct AggregateMetric
{
    double mean{};
    double minimum{};
    double maximum{};
    double standardDeviation{};
};

struct AggregatedExperimentResult
{
    std::string variantName;
    std::size_t runCount{};

    AggregateMetric vehiclesSpawned;
    AggregateMetric vehiclesArrived;
    AggregateMetric totalReroutes;
    AggregateMetric averageTravelTimeSeconds;
    AggregateMetric averageWaitingTimeSeconds;
    AggregateMetric averageSpeedMetersPerSecond;
    AggregateMetric totalDistanceMeters;
    AggregateMetric averageRouteLengthMeters;
    AggregateMetric peakActiveVehicles;
};

class ExperimentAggregator final
{
  public:
    [[nodiscard]] static std::vector<AggregatedExperimentResult>
    aggregate(std::span<const ExperimentRunResult> runResults);
};

} // namespace trafficsim

#endif // TRAFFICSIM_EXPERIMENTS_EXPERIMENT_AGGREGATOR_H