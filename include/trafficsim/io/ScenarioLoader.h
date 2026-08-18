#ifndef TRAFFICSIM_IO_SCENARIO_LOADER_H
#define TRAFFICSIM_IO_SCENARIO_LOADER_H

#include "trafficsim/io/Scenario.h"

#include <filesystem>
#include <string_view>

namespace trafficsim
{

class ScenarioLoader final
{
  public:
    [[nodiscard]] static Scenario loadFromFile(const std::filesystem::path &filePath);
    [[nodiscard]] static Scenario loadFromJson(std::string_view jsonText);
};

} // namespace trafficsim

#endif // TRAFFICSIM_IO_SCENARIO_LOADER_H