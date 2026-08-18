#ifndef TRAFFICSIM_VEHICLES_VEHICLE_TYPES_H
#define TRAFFICSIM_VEHICLES_VEHICLE_TYPES_H

#include <cstdint>

namespace trafficsim
{

using VehicleId = std::uint64_t;

enum class VehicleState : std::uint8_t
{
    Spawning,
    Driving,
    Waiting,
    StoppedAtLight,
    Rerouting,
    Arrived,
};

struct VehicleDynamics
{
    double maximumSpeedMetersPerSecond;
    double accelerationMetersPerSecondSquared;
    double decelerationMetersPerSecondSquared;

    void validate() const;
};

struct VehicleFollowingConfig
{
    double minimumDistanceMeters{2.0};
    double reactionTimeSeconds{1.0};

    void validate() const;
};

struct VehicleFollowingConstraint
{
    double maximumPositionMeters;
    double desiredSpeedLimitMetersPerSecond;

    void validate(double currentPositionMeters) const;
};

} // namespace trafficsim

#endif // TRAFFICSIM_VEHICLES_VEHICLE_TYPES_H
