#ifndef TRAFFICSIM_STATISTICS_CONSOLE_REPORTER_H
#define TRAFFICSIM_STATISTICS_CONSOLE_REPORTER_H

#include "trafficsim/statistics/StatisticsTypes.h"

#include <iosfwd>
#include <span>

namespace trafficsim
{

class ConsoleReporter final
{
  public:
    static void write(std::ostream &output, double simulationTimeSeconds,
                      const SimulationSummary &summary, std::span<const RoadResult> roadResults);
};

} // namespace trafficsim

#endif // TRAFFICSIM_STATISTICS_CONSOLE_REPORTER_H