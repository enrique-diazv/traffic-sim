#ifndef TRAFFICSIM_CORE_RANDOM_GENERATOR_H
#define TRAFFICSIM_CORE_RANDOM_GENERATOR_H

#include <cstdint>
#include <random>

namespace trafficsim
{

class RandomGenerator final
{
  public:
    explicit RandomGenerator(std::uint64_t seed);

    [[nodiscard]] int randomInt(int minimum, int maximum);
    [[nodiscard]] double randomDouble(double minimum, double maximum);

  private:
    std::mt19937_64 engine_;
};

} // namespace trafficsim

#endif // TRAFFICSIM_CORE_RANDOM_GENERATOR_H
