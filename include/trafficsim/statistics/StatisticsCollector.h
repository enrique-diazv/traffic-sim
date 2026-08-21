#ifndef TRAFFICSIM_STATISTICS_STATISTICS_COLLECTOR_H
#define TRAFFICSIM_STATISTICS_STATISTICS_COLLECTOR_H

#include "trafficsim/statistics/StatisticsTypes.h"
#include "trafficsim/traffic/RoadTrafficMetrics.h"
#include "trafficsim/vehicles/Vehicle.h"

#include <cstddef>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace trafficsim
{

class StatisticsCollector final
{
  public:
    void recordSpawned(std::size_t count = 1);
    void observeActiveVehicles(std::span<const Vehicle> vehicles) noexcept;
    void observeRoads(double deltaSeconds, std::span<const RoadTrafficMetrics> roadMetrics);
    void recordCompletedVehicle(const Vehicle &vehicle);
    void reset() noexcept;

    [[nodiscard]] SimulationSummary summary() const noexcept;
    [[nodiscard]] std::span<const VehicleResult> vehicleResults() const noexcept;
    [[nodiscard]] std::vector<RoadResult> roadResults() const;

  private:
    struct RoadAccumulator
    {
        RoadResult result;
        double totalSpeedMetersPerSecond{};
        std::size_t speedObservationCount{};
        double totalOccupancy{};
        std::size_t stepObservationCount{};
    };

    std::size_t vehiclesSpawned_{};
    std::size_t peakActiveVehicles_{};
    std::vector<VehicleResult> vehicleResults_;
    std::unordered_set<VehicleId> completedVehicleIds_;
    std::unordered_map<RoadId, RoadAccumulator> roadAccumulators_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_STATISTICS_STATISTICS_COLLECTOR_H