#include "ProcessCpuTime.h"

#include <ctime>
#include <stdexcept>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace trafficsim::benchmarking
{

double currentProcessCpuTimeMilliseconds()
{
#if defined(_WIN32)
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};

    if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime) ==
        FALSE)
    {
        throw std::runtime_error{"Process CPU time is unavailable"};
    }

    ULARGE_INTEGER kernelValue{};
    kernelValue.LowPart = kernelTime.dwLowDateTime;
    kernelValue.HighPart = kernelTime.dwHighDateTime;

    ULARGE_INTEGER userValue{};
    userValue.LowPart = userTime.dwLowDateTime;
    userValue.HighPart = userTime.dwHighDateTime;

    constexpr double fileTimeUnitsPerMillisecond{10'000.0};

    return static_cast<double>(kernelValue.QuadPart + userValue.QuadPart) /
           fileTimeUnitsPerMillisecond;
#else
    const auto cpuTime = std::clock();

    if (cpuTime == static_cast<std::clock_t>(-1))
    {
        throw std::runtime_error{"Process CPU time is unavailable"};
    }

    return (static_cast<double>(cpuTime) * 1000.0) / static_cast<double>(CLOCKS_PER_SEC);
#endif
}

} // namespace trafficsim::benchmarking