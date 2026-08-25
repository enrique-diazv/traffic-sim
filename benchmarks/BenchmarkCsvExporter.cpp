#include "BenchmarkCsvExporter.h"

#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>

namespace trafficsim::benchmarking
{

void BenchmarkCsvExporter::write(std::ostream &output, std::span<const BenchmarkSample> samples)
{
    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();

    output << std::defaultfloat << std::setprecision(15);
    output << "benchmark,vehicle_count,repetition_index,"
              "operation_count,wall_time_milliseconds,"
              "cpu_time_milliseconds,cpu_utilization_percent,"
              "simulated_seconds,"
              "simulated_seconds_per_real_second\n";

    for (const auto &sample : samples)
    {
        output << benchmarkName(sample.kind) << ',' << sample.vehicleCount << ','
               << sample.repetitionIndex << ',' << sample.operationCount << ','
               << sample.wallTimeMilliseconds << ',' << sample.cpuTimeMilliseconds << ','
               << sample.cpuUtilizationPercent << ',' << sample.simulatedSeconds << ','
               << sample.simulatedSecondsPerRealSecond << '\n';
    }

    output.flags(previousFlags);
    output.precision(previousPrecision);

    if (!output)
    {
        throw std::runtime_error{"Could not write benchmark CSV output"};
    }
}

void BenchmarkCsvExporter::exportToFile(const std::filesystem::path &filePath,
                                        std::span<const BenchmarkSample> samples)
{
    if (filePath.empty())
    {
        throw std::invalid_argument{
            "Benchmark CSV output path must not be empty",
        };
    }

    if (!filePath.parent_path().empty())
    {
        std::filesystem::create_directories(filePath.parent_path());
    }

    std::ofstream output{filePath};

    if (!output.is_open())
    {
        throw std::runtime_error{
            "Could not open benchmark CSV output file: " + filePath.string(),
        };
    }

    write(output, samples);
}

} // namespace trafficsim::benchmarking