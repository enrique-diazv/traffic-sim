#ifndef TRAFFICSIM_TRAFFIC_TRAFFIC_LIGHT_H
#define TRAFFICSIM_TRAFFIC_TRAFFIC_LIGHT_H

#include <cstdint>

namespace trafficsim
{

enum class TrafficLightState : std::uint8_t
{
    Green,
    Yellow,
    Red,
};

struct TrafficLightTimings
{
    double greenSeconds;
    double yellowSeconds;
    double redSeconds;

    void validate() const;
};

class TrafficLight final
{
  public:
    explicit TrafficLight(TrafficLightTimings timings,
                          TrafficLightState initialState = TrafficLightState::Green);

    void update(double deltaSeconds);
    void reset() noexcept;

    [[nodiscard]] TrafficLightState state() const noexcept;
    [[nodiscard]] double elapsedInStateSeconds() const noexcept;
    [[nodiscard]] double remainingInStateSeconds() const noexcept;
    [[nodiscard]] const TrafficLightTimings &timings() const noexcept;

  private:
    [[nodiscard]] double stateDurationSeconds() const noexcept;
    void transitionToNextState() noexcept;

    TrafficLightTimings timings_;
    TrafficLightState initialState_;
    TrafficLightState state_;
    double elapsedInStateSeconds_{};
};

} // namespace trafficsim

#endif // TRAFFICSIM_TRAFFIC_TRAFFIC_LIGHT_H
