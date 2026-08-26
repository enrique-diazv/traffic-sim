#include "BenchmarkTypes.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace trafficsim::benchmarking
{

void BenchmarkConfig::validate() const
{
    if (vehicleCounts.empty())
    {
        throw std::invalid_argument{"Benchmark vehicle counts must not be empty"};
    }

    if (repetitions == 0U || roadLookupOperationsPerVehicle == 0U || vehicleUpdateSteps == 0U ||
        batchRunCount == 0U || parallelWorkerCount == 0U)
    {
        throw std::invalid_argument{"Benchmark operation counts must be positive"};
    }

    std::size_t previousVehicleCount{};

    for (const auto vehicleCount : vehicleCounts)
    {
        if (vehicleCount == 0U)
        {
            throw std::invalid_argument{"Benchmark vehicle counts must be positive"};
        }

        if (vehicleCount <= previousVehicleCount)
        {
            throw std::invalid_argument{
                "Benchmark vehicle counts must be strictly increasing",
            };
        }

        if (vehicleCount > std::numeric_limits<std::size_t>::max() / roadLookupOperationsPerVehicle)
        {
            throw std::invalid_argument{
                "Benchmark road lookup operation count would overflow",
            };
        }

        if (vehicleCount > std::numeric_limits<std::size_t>::max() / vehicleUpdateSteps)
        {
            throw std::invalid_argument{
                "Benchmark vehicle update operation count would overflow",
            };
        }

        previousVehicleCount = vehicleCount;
    }

    if (!std::isfinite(simulationTimeStepSeconds) || simulationTimeStepSeconds <= 0.0 ||
        !std::isfinite(fullSimulationDurationSeconds) || fullSimulationDurationSeconds <= 0.0 ||
        fullSimulationDurationSeconds < simulationTimeStepSeconds)
    {
        throw std::invalid_argument{"Benchmark simulation durations must be valid"};
    }
}

std::string_view benchmarkName(BenchmarkKind kind)
{
    switch (kind)
    {
    case BenchmarkKind::RoadLookup:
        return "road_lookup";
    case BenchmarkKind::Routing:
        return "routing";
    case BenchmarkKind::VehicleUpdate:
        return "vehicle_update";
    case BenchmarkKind::Statistics:
        return "statistics";
    case BenchmarkKind::FullSimulation:
        return "full_simulation";
    case BenchmarkKind::BatchSequential:
        return "batch_sequential";
    case BenchmarkKind::BatchParallel:
        return "batch_parallel";
    }

    throw std::invalid_argument{"Unknown benchmark kind"};
}

} // namespace trafficsim::benchmarking