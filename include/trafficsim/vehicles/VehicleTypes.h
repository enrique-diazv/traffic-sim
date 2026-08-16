#ifndef TRAFFICSIM_VEHICLES_VEHICLE_TYPES_H
#define TRAFFICSIM_VEHICLES_VEHICLE_TYPES_H

#include <cstdint>

namespace trafficsim
{

using VehicleId = std::uint64_t;

enum class VehicleState
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

} // namespace trafficsim

#endif // TRAFFICSIM_VEHICLES_VEHICLE_TYPES_H