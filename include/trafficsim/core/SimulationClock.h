#ifndef TRAFFICSIM_CORE_SIMULATION_CLOCK_H
#define TRAFFICSIM_CORE_SIMULATION_CLOCK_H

#include <cstdint>

namespace trafficsim
{

class SimulationClock final
{
  public:
    explicit SimulationClock(double timeStepSeconds);

    void advance();
    void reset() noexcept;

    [[nodiscard]] double currentTimeSeconds() const noexcept;
    [[nodiscard]] double timeStepSeconds() const noexcept;
    [[nodiscard]] std::uint64_t tickCount() const noexcept;

  private:
    double timeStepSeconds_;
    std::uint64_t tickCount_{};
};

} // namespace trafficsim

#endif // TRAFFICSIM_CORE_SIMULATION_CLOCK_H
