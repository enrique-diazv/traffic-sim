#ifndef TRAFFICSIM_VEHICLES_VEHICLE_H
#define TRAFFICSIM_VEHICLES_VEHICLE_H

#include "trafficsim/network/RoadNetwork.h"
#include "trafficsim/routing/Route.h"
#include "trafficsim/vehicles/VehicleTypes.h"

#include <optional>

namespace trafficsim
{

class TrafficManager;

class Vehicle final
{
  public:
    Vehicle(VehicleId vehicleId, IntersectionId origin, IntersectionId destination, Route route,
            VehicleDynamics dynamics);

    [[nodiscard]] VehicleId id() const noexcept;
    [[nodiscard]] IntersectionId origin() const noexcept;
    [[nodiscard]] IntersectionId destination() const noexcept;

    [[nodiscard]] VehicleState state() const noexcept;
    [[nodiscard]] std::optional<RoadId> currentRoad() const noexcept;
    [[nodiscard]] const Route &route() const noexcept;

    [[nodiscard]] double positionMeters() const noexcept;
    [[nodiscard]] double speedMetersPerSecond() const noexcept;
    [[nodiscard]] double maximumSpeedMetersPerSecond() const noexcept;

    [[nodiscard]] std::optional<double> spawnTimeSeconds() const noexcept;
    [[nodiscard]] std::optional<double> arrivalTimeSeconds() const noexcept;
    [[nodiscard]] std::optional<double> travelTimeSeconds() const noexcept;

    [[nodiscard]] double waitingTimeSeconds() const noexcept;

    [[nodiscard]] bool start(const RoadNetwork &network, double spawnTimeSeconds = 0.0);
    [[nodiscard]] bool reroute(const RoadNetwork &network, Route continuation);
    void update(double deltaSeconds, const RoadNetwork &network,
                const TrafficManager *trafficManager = nullptr,
                const VehicleFollowingConstraint *followingConstraint = nullptr);

  private:
    void resumeFromTrafficLight(const TrafficManager *trafficManager);
    void resumeFromQueue(const VehicleFollowingConstraint *followingConstraint);
    void updateSpeed(double desiredSpeed, double deltaSeconds) noexcept;
    void applyFollowingConstraint(const VehicleFollowingConstraint *followingConstraint);
    void advanceAcrossCompletedRoads(const RoadNetwork &network,
                                     const TrafficManager *trafficManager);

    VehicleId id_;
    IntersectionId origin_;
    IntersectionId destination_;
    Route route_;
    VehicleDynamics dynamics_;
    VehicleState state_{VehicleState::Spawning};
    double positionMeters_{};
    double speedMetersPerSecond_{};

    std::optional<double> spawnTimeSeconds_;
    std::optional<double> arrivalTimeSeconds_;
    double elapsedTravelTimeSeconds_{};
    double waitingTimeSeconds_{};
};

} // namespace trafficsim

#endif // TRAFFICSIM_VEHICLES_VEHICLE_H
