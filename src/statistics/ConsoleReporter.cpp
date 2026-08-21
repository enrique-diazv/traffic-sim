#include "trafficsim/statistics/ConsoleReporter.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>

namespace trafficsim
{

void ConsoleReporter::write(std::ostream &output, double simulationTimeSeconds,
                            const SimulationSummary &summary,
                            std::span<const RoadResult> roadResults)
{
    if (!std::isfinite(simulationTimeSeconds) || simulationTimeSeconds < 0.0)
    {
        throw std::invalid_argument{"Report simulation time must be finite and non-negative"};
    }

    const auto congestedRoadCount =
        std::ranges::count_if(roadResults, [](const RoadResult &road)
                              { return road.peakCongestionState >= CongestionState::Congested; });

    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();

    output << std::fixed << std::setprecision(2);
    output << "TrafficSim Results\n\n";
    output << std::left << std::setw(26) << "Simulation Time:" << simulationTimeSeconds << " s\n";
    output << std::left << std::setw(26) << "Vehicles Spawned:" << summary.vehiclesSpawned << '\n';
    output << std::left << std::setw(26) << "Vehicles Arrived:" << summary.vehiclesArrived << '\n';
    output << std::left << std::setw(26)
           << "Average Travel Time:" << summary.averageTravelTimeSeconds << " s\n";
    output << std::left << std::setw(26)
           << "Average Waiting Time:" << summary.averageWaitingTimeSeconds << " s\n";
    output << std::left << std::setw(26)
           << "Average Speed:" << summary.averageSpeedMetersPerSecond * 3.6 << " km/h\n";
    output << std::left << std::setw(26) << "Total Distance:" << summary.totalDistanceMeters
           << " m\n";
    output << std::left << std::setw(26)
           << "Average Route Length:" << summary.averageRouteLengthMeters << " m\n";
    output << std::left << std::setw(26) << "Peak Active Vehicles:" << summary.peakActiveVehicles
           << '\n';
    output << std::left << std::setw(26) << "Congested Roads:" << congestedRoadCount << '\n';

    output.flags(previousFlags);
    output.precision(previousPrecision);
}

} // namespace trafficsim