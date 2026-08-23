#ifndef TRAFFICSIM_VISUALIZATION_SIMULATION_VISUALIZER_H
#define TRAFFICSIM_VISUALIZATION_SIMULATION_VISUALIZER_H

#include "trafficsim/core/Simulation.h"

#include <SFML/Graphics.hpp>

#include <filesystem>

namespace trafficsim::visualization
{

struct VisualizationConfig
{
    sf::Vector2u windowSize{1280U, 720U};
    float roadWidth{6.0F};
    float intersectionRadius{8.0F};
    float vehicleRadius{4.0F};
    float trafficLightRadius{3.0F};
    float worldPadding{30.0F};
    float cameraPanSpeed{0.75F};
    float statusPanelWidth{430.0F};
    unsigned int statusCharacterSize{18U};
    std::filesystem::path fontPath{"assets/fonts/RobotoMono-Regular.ttf"};
};

class SimulationVisualizer final
{
  public:
    explicit SimulationVisualizer(Simulation &simulation, VisualizationConfig config = {});

    void run();

  private:
    void processEvents();
    void handleKeyPressed(const sf::Event::KeyPressed &event);
    void handleMouseWheelScrolled(const sf::Event::MouseWheelScrolled &event);
    void updateCamera(double frameSeconds);
    void update();
    void fitWorldView();
    void render();
    void drawRoads();
    void drawIntersections();
    void drawTrafficLights();
    void drawVehicles();
    void drawStatusPanel();

    Simulation &simulation_;
    VisualizationConfig config_;
    sf::RenderWindow window_;
    sf::View worldView_;
    sf::Font font_;
    sf::Clock frameClock_;
    double accumulatedSimulationSeconds_{};
    bool paused_{true};
    float zoomLevel_{1.0F};
};

} // namespace trafficsim::visualization

#endif // TRAFFICSIM_VISUALIZATION_SIMULATION_VISUALIZER_H