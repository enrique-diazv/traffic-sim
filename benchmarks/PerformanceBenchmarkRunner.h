#ifndef TRAFFICSIM_BENCHMARKS_PERFORMANCE_BENCHMARK_RUNNER_H
#define TRAFFICSIM_BENCHMARKS_PERFORMANCE_BENCHMARK_RUNNER_H

#include "BenchmarkTypes.h"

#include <iosfwd>
#include <vector>

namespace trafficsim::benchmarking
{

class PerformanceBenchmarkRunner final
{
  public:
    [[nodiscard]] static std::vector<BenchmarkSample> run(const BenchmarkConfig &config,
                                                          std::ostream &progressOutput);
};

} // namespace trafficsim::benchmarking

#endif // TRAFFICSIM_BENCHMARKS_PERFORMANCE_BENCHMARK_RUNNER_H