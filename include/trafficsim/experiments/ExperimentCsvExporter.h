#ifndef TRAFFICSIM_EXPERIMENTS_EXPERIMENT_CSV_EXPORTER_H
#define TRAFFICSIM_EXPERIMENTS_EXPERIMENT_CSV_EXPORTER_H

#include "trafficsim/experiments/BatchExperimentRunner.h"
#include "trafficsim/experiments/ExperimentAggregator.h"

#include <filesystem>
#include <iosfwd>
#include <span>

namespace trafficsim
{

class ExperimentCsvExporter final
{
  public:
    static void writeRunResults(std::ostream &output,
                                std::span<const ExperimentRunResult> runResults);

    static void
    writeAggregatedResults(std::ostream &output,
                           std::span<const AggregatedExperimentResult> aggregatedResults);

    static void exportToDirectory(const std::filesystem::path &directory,
                                  std::span<const ExperimentRunResult> runResults,
                                  std::span<const AggregatedExperimentResult> aggregatedResults);
};

} // namespace trafficsim

#endif // TRAFFICSIM_EXPERIMENTS_EXPERIMENT_CSV_EXPORTER_H