#include "trafficsim/core/RandomGenerator.h"

#include <cmath>
#include <stdexcept>

namespace trafficsim
{

RandomGenerator::RandomGenerator(std::uint64_t seed) : engine_{seed} {}

int RandomGenerator::randomInt(int minimum, int maximum)
{
    if (minimum > maximum)
    {
        throw std::invalid_argument{"Random integer minimum cannot exceed maximum"};
    }

    std::uniform_int_distribution<int> distribution{minimum, maximum};
    return distribution(engine_);
}

double RandomGenerator::randomDouble(double minimum, double maximum)
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum)
    {
        throw std::invalid_argument{"Random double bounds must be finite and ordered"};
    }

    std::uniform_real_distribution<double> distribution{minimum, maximum};
    return distribution(engine_);
}

} // namespace trafficsim
