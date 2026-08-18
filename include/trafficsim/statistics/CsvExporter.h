#ifndef TRAFFICSIM_STATISTICS_CSV_EXPORTER_H
#define TRAFFICSIM_STATISTICS_CSV_EXPORTER_H

#include "trafficsim/statistics/StatisticsTypes.h"

#include <filesystem>
#include <iosfwd>
#include <span>

namespace trafficsim
{

class CsvExporter final
{
  public:
    static void writeSimulationSummary(std::ostream &output, const SimulationSummary &summary);
    static void writeVehicleResults(std::ostream &output,
                                    std::span<const VehicleResult> vehicleResults);
    static void writeRoadResults(std::ostream &output, std::span<const RoadResult> roadResults);

    static void exportToDirectory(const std::filesystem::path &directory,
                                  const SimulationSummary &summary,
                                  std::span<const VehicleResult> vehicleResults,
                                  std::span<const RoadResult> roadResults);
};

} // namespace trafficsim

#endif // TRAFFICSIM_STATISTICS_CSV_EXPORTER_H