#ifndef TRAFFICSIM_BENCHMARKS_BENCHMARK_CSV_EXPORTER_H
#define TRAFFICSIM_BENCHMARKS_BENCHMARK_CSV_EXPORTER_H

#include "BenchmarkTypes.h"

#include <filesystem>
#include <iosfwd>
#include <span>

namespace trafficsim::benchmarking
{

class BenchmarkCsvExporter final
{
  public:
    static void write(std::ostream &output, std::span<const BenchmarkSample> samples);

    static void exportToFile(const std::filesystem::path &filePath,
                             std::span<const BenchmarkSample> samples);
};

} // namespace trafficsim::benchmarking

#endif // TRAFFICSIM_BENCHMARKS_BENCHMARK_CSV_EXPORTER_H