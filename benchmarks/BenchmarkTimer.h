#ifndef TRAFFICSIM_BENCHMARKS_BENCHMARK_TIMER_H
#define TRAFFICSIM_BENCHMARKS_BENCHMARK_TIMER_H

#include "ProcessCpuTime.h"

#include <chrono>
#include <functional>
#include <utility>

namespace trafficsim::benchmarking
{

struct TimingResult
{
    double wallTimeMilliseconds{};
    double cpuTimeMilliseconds{};
    double cpuUtilizationPercent{};
};

class BenchmarkTimer final
{
  public:
    template <typename Operation> [[nodiscard]] static TimingResult measure(Operation &&operation)
    {
        const auto cpuStartMilliseconds = currentProcessCpuTimeMilliseconds();
        const auto wallStart = std::chrono::steady_clock::now();

        std::invoke(std::forward<Operation>(operation));

        const auto wallEnd = std::chrono::steady_clock::now();
        const auto cpuEndMilliseconds = currentProcessCpuTimeMilliseconds();

        const auto wallTimeMilliseconds =
            std::chrono::duration<double, std::milli>(wallEnd - wallStart).count();

        const auto cpuTimeMilliseconds = cpuEndMilliseconds - cpuStartMilliseconds;

        const auto cpuUtilizationPercent =
            wallTimeMilliseconds > 0.0 ? (cpuTimeMilliseconds / wallTimeMilliseconds) * 100.0 : 0.0;

        return {
            .wallTimeMilliseconds = wallTimeMilliseconds,
            .cpuTimeMilliseconds = cpuTimeMilliseconds,
            .cpuUtilizationPercent = cpuUtilizationPercent,
        };
    }
};

} // namespace trafficsim::benchmarking

#endif // TRAFFICSIM_BENCHMARKS_BENCHMARK_TIMER_H