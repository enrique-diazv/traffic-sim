#ifndef TRAFFICSIM_BENCHMARKS_BENCHMARK_FIXTURES_H
#define TRAFFICSIM_BENCHMARKS_BENCHMARK_FIXTURES_H

#include "trafficsim/core/Simulation.h"
#include "trafficsim/experiments/BatchExperimentConfig.h"
#include "trafficsim/io/Scenario.h"
#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/traffic/RoadTrafficMetrics.h"
#include "trafficsim/vehicles/VehicleManager.h"

#include <cstddef>
#include <vector>

namespace trafficsim::benchmarking
{

[[nodiscard]] RoadNetwork createLinearNetwork(std::size_t roadCount, double roadLengthMeters);

[[nodiscard]] VehicleManager createVehicleManager(std::size_t vehicleCount,
                                                  const RoadNetwork &network);

[[nodiscard]] std::vector<RoadTrafficMetrics> createRoadMetrics(std::size_t metricCount);

[[nodiscard]] Scenario createBenchmarkScenario(std::size_t vehicleCount, double timeStepSeconds,
                                               double durationSeconds);

[[nodiscard]] BatchExperimentConfig createBatchExperimentConfig(const Scenario &scenario,
                                                                std::size_t runCount);

[[nodiscard]] Simulation createSimulation(std::size_t vehicleCount, double timeStepSeconds,
                                          double durationSeconds);

} // namespace trafficsim::benchmarking

#endif // TRAFFICSIM_BENCHMARKS_BENCHMARK_FIXTURES_H