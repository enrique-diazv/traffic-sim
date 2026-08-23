#include "SimulationVisualizer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace trafficsim::visualization
{

namespace
{

VisualizationConfig validatedConfig(VisualizationConfig config)
{
    if (config.windowSize.x == 0U || config.windowSize.y == 0U)
    {
        throw std::invalid_argument{"Visualizer window dimensions must be positive"};
    }

    if (!std::isfinite(config.roadWidth) || config.roadWidth <= 0.0F ||
        !std::isfinite(config.intersectionRadius) || config.intersectionRadius <= 0.0F ||
        !std::isfinite(config.vehicleRadius) || config.vehicleRadius <= 0.0F ||
        !std::isfinite(config.trafficLightRadius) || config.trafficLightRadius <= 0.0F ||
        !std::isfinite(config.cameraPanSpeed) || config.cameraPanSpeed <= 0.0F ||
        !std::isfinite(config.statusPanelWidth) || config.statusPanelWidth <= 0.0F ||
        !std::isfinite(config.worldPadding) || config.worldPadding < 0.0F)
    {
        throw std::invalid_argument{"Visualizer dimensions must be finite and valid"};
    }

    if (config.statusCharacterSize == 0U || config.fontPath.empty())
    {
        throw std::invalid_argument{"Visualizer font configuration must be valid"};
    }

    return config;
}

sf::Vector2f toVector(Position position)
{
    return {
        static_cast<float>(position.x),
        static_cast<float>(position.y),
    };
}

std::optional<sf::Vector2f> vehiclePosition(const Vehicle &vehicle, const RoadNetwork &network)
{
    const auto currentRoadId = vehicle.currentRoad();

    if (!currentRoadId.has_value())
    {
        return std::nullopt;
    }

    const auto &road = network.getRoad(*currentRoadId);
    const auto origin = toVector(network.getIntersection(road.origin()).position());
    const auto destination = toVector(network.getIntersection(road.destination()).position());
    const auto progress = std::clamp(vehicle.positionMeters() / road.lengthMeters(), 0.0, 1.0);
    const auto displayProgress = static_cast<float>(progress);

    return sf::Vector2f{
        origin.x + ((destination.x - origin.x) * displayProgress),
        origin.y + ((destination.y - origin.y) * displayProgress),
    };
}

sf::Color trafficLightColor(TrafficLightState state)
{
    switch (state)
    {
    case TrafficLightState::Green:
        return sf::Color{62U, 214U, 114U};

    case TrafficLightState::Yellow:
        return sf::Color{255U, 204U, 64U};

    case TrafficLightState::Red:
        return sf::Color{242U, 84U, 84U};
    }

    throw std::invalid_argument{"Unknown traffic light state"};
}

} // namespace

SimulationVisualizer::SimulationVisualizer(Simulation &simulation, VisualizationConfig config)
    : simulation_{simulation}, config_{validatedConfig(std::move(config))},
      window_{sf::VideoMode{config_.windowSize}, "TrafficSim Visualizer"}, font_{config_.fontPath}
{
    window_.setFramerateLimit(60U);
    window_.setKeyRepeatEnabled(false);
    fitWorldView();
}

void SimulationVisualizer::run()
{
    while (window_.isOpen())
    {
        processEvents();
        update();
        render();
    }
}

void SimulationVisualizer::processEvents()
{
    while (const std::optional event = window_.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window_.close();
        }
        else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            handleKeyPressed(*keyPressed);
        }
        else if (const auto *mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            handleMouseWheelScrolled(*mouseWheel);
        }
        else if (const auto *resized = event->getIf<sf::Event::Resized>())
        {
            if (resized->size.x > 0U && resized->size.y > 0U)
            {
                config_.windowSize = resized->size;
                fitWorldView();
            }
        }
    }
}

void SimulationVisualizer::handleKeyPressed(const sf::Event::KeyPressed &event)
{
    switch (event.code)
    {
    case sf::Keyboard::Key::Escape:
        window_.close();
        break;

    case sf::Keyboard::Key::Space:
        paused_ = !paused_;
        accumulatedSimulationSeconds_ = 0.0;
        static_cast<void>(frameClock_.restart());
        break;

    case sf::Keyboard::Key::Right:
        if (paused_ && !simulation_.finished())
        {
            simulation_.step();
        }
        break;

    case sf::Keyboard::Key::R:
        simulation_.reset();
        accumulatedSimulationSeconds_ = 0.0;
        paused_ = true;
        static_cast<void>(frameClock_.restart());
        break;

    case sf::Keyboard::Key::F:
        fitWorldView();
        break;

    default:
        break;
    }
}

void SimulationVisualizer::handleMouseWheelScrolled(const sf::Event::MouseWheelScrolled &event)
{
    constexpr float zoomStep{0.85F};
    constexpr float minimumZoom{0.1F};
    constexpr float maximumZoom{10.0F};

    if (event.delta == 0.0F)
    {
        return;
    }

    const auto requestedZoom =
        std::clamp(zoomLevel_ * std::pow(zoomStep, event.delta), minimumZoom, maximumZoom);
    const auto relativeZoom = requestedZoom / zoomLevel_;

    worldView_.zoom(relativeZoom);
    zoomLevel_ = requestedZoom;
}

void SimulationVisualizer::updateCamera(double frameSeconds)
{
    sf::Vector2f direction{};

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
    {
        direction.x -= 1.0F;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
    {
        direction.x += 1.0F;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
    {
        direction.y -= 1.0F;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
    {
        direction.y += 1.0F;
    }

    const auto directionLength = std::hypot(direction.x, direction.y);

    if (directionLength <= 0.0F)
    {
        return;
    }

    const auto distance =
        worldView_.getSize().y * config_.cameraPanSpeed * static_cast<float>(frameSeconds);

    worldView_.move({
        (direction.x / directionLength) * distance,
        (direction.y / directionLength) * distance,
    });
}

void SimulationVisualizer::update()
{
    constexpr double maximumFrameSeconds{0.25};

    const auto frameSeconds =
        std::min(static_cast<double>(frameClock_.restart().asSeconds()), maximumFrameSeconds);

    updateCamera(frameSeconds);

    if (paused_ || simulation_.finished())
    {
        accumulatedSimulationSeconds_ = 0.0;
        return;
    }

    accumulatedSimulationSeconds_ += frameSeconds;
    const auto timeStepSeconds = simulation_.config().timeStepSeconds;

    while (accumulatedSimulationSeconds_ >= timeStepSeconds && !simulation_.finished())
    {
        simulation_.step();
        accumulatedSimulationSeconds_ -= timeStepSeconds;
    }
}

void SimulationVisualizer::fitWorldView()
{
    const auto intersectionIds = simulation_.roadNetwork().intersectionIds();

    zoomLevel_ = 1.0F;

    if (intersectionIds.empty())
    {
        worldView_.setCenter({0.0F, 0.0F});
        worldView_.setSize({100.0F, 100.0F});
        window_.setView(worldView_);
        return;
    }

    auto minimumX = std::numeric_limits<float>::max();
    auto minimumY = std::numeric_limits<float>::max();
    auto maximumX = std::numeric_limits<float>::lowest();
    auto maximumY = std::numeric_limits<float>::lowest();

    for (const auto intersectionId : intersectionIds)
    {
        const auto position =
            toVector(simulation_.roadNetwork().getIntersection(intersectionId).position());

        minimumX = std::min(minimumX, position.x);
        minimumY = std::min(minimumY, position.y);
        maximumX = std::max(maximumX, position.x);
        maximumY = std::max(maximumY, position.y);
    }

    auto width = std::max(maximumX - minimumX, 1.0F) + (2.0F * config_.worldPadding);
    auto height = std::max(maximumY - minimumY, 1.0F) + (2.0F * config_.worldPadding);

    const auto windowAspect =
        static_cast<float>(config_.windowSize.x) / static_cast<float>(config_.windowSize.y);

    if ((width / height) > windowAspect)
    {
        height = width / windowAspect;
    }
    else
    {
        width = height * windowAspect;
    }

    worldView_.setCenter({
        (minimumX + maximumX) / 2.0F,
        (minimumY + maximumY) / 2.0F,
    });
    worldView_.setSize({width, height});
    window_.setView(worldView_);
}

void SimulationVisualizer::render()
{
    window_.clear(sf::Color{24U, 28U, 36U});
    window_.setView(worldView_);

    drawRoads();
    drawIntersections();
    drawTrafficLights();
    drawVehicles();
    drawStatusPanel();

    window_.display();
}

void SimulationVisualizer::drawRoads()
{
    const auto &network = simulation_.roadNetwork();

    for (const auto roadId : network.roadIds())
    {
        const auto &road = network.getRoad(roadId);
        const auto origin = toVector(network.getIntersection(road.origin()).position());
        const auto destination = toVector(network.getIntersection(road.destination()).position());
        const sf::Vector2f difference = destination - origin;
        const auto displayLength = std::hypot(difference.x, difference.y);

        if (displayLength <= 0.0F)
        {
            continue;
        }

        sf::RectangleShape roadShape{
            sf::Vector2f{displayLength, config_.roadWidth},
        };

        roadShape.setOrigin({0.0F, config_.roadWidth / 2.0F});
        roadShape.setPosition(origin);
        roadShape.setRotation(sf::radians(std::atan2(difference.y, difference.x)));
        roadShape.setFillColor(sf::Color{82U, 91U, 102U});

        window_.draw(roadShape);
    }
}

void SimulationVisualizer::drawIntersections()
{
    const auto &network = simulation_.roadNetwork();
    sf::CircleShape intersectionShape{config_.intersectionRadius};

    intersectionShape.setOrigin({
        config_.intersectionRadius,
        config_.intersectionRadius,
    });
    intersectionShape.setFillColor(sf::Color{241U, 163U, 64U});
    intersectionShape.setOutlineColor(sf::Color{255U, 224U, 168U});
    intersectionShape.setOutlineThickness(2.0F);

    for (const auto intersectionId : network.intersectionIds())
    {
        intersectionShape.setPosition(toVector(network.getIntersection(intersectionId).position()));
        window_.draw(intersectionShape);
    }
}

void SimulationVisualizer::drawTrafficLights()
{
    const auto &network = simulation_.roadNetwork();
    sf::CircleShape lightShape{config_.trafficLightRadius};

    lightShape.setOrigin({
        config_.trafficLightRadius,
        config_.trafficLightRadius,
    });
    lightShape.setOutlineColor(sf::Color{235U, 239U, 244U});
    lightShape.setOutlineThickness(1.0F);

    for (const auto roadId : network.roadIds())
    {
        const auto lightState = simulation_.trafficManager().stateForRoad(roadId);

        if (!lightState.has_value())
        {
            continue;
        }

        const auto &road = network.getRoad(roadId);
        const auto origin = toVector(network.getIntersection(road.origin()).position());
        const auto destination = toVector(network.getIntersection(road.destination()).position());
        const sf::Vector2f difference = destination - origin;
        const auto displayLength = std::hypot(difference.x, difference.y);

        if (displayLength <= 0.0F)
        {
            continue;
        }

        const sf::Vector2f direction{
            difference.x / displayLength,
            difference.y / displayLength,
        };
        const auto offset = config_.intersectionRadius + config_.trafficLightRadius + 2.0F;

        lightShape.setPosition({
            destination.x - (direction.x * offset),
            destination.y - (direction.y * offset),
        });
        lightShape.setFillColor(trafficLightColor(*lightState));
        window_.draw(lightShape);
    }
}

void SimulationVisualizer::drawVehicles()
{
    const auto &network = simulation_.roadNetwork();
    sf::CircleShape vehicleShape{config_.vehicleRadius};

    vehicleShape.setOrigin({
        config_.vehicleRadius,
        config_.vehicleRadius,
    });
    vehicleShape.setFillColor(sf::Color{72U, 202U, 228U});
    vehicleShape.setOutlineColor(sf::Color{210U, 250U, 255U});
    vehicleShape.setOutlineThickness(1.5F);

    for (const auto &vehicle : simulation_.vehicleManager().vehicles())
    {
        const auto position = vehiclePosition(vehicle, network);

        if (!position.has_value())
        {
            continue;
        }

        vehicleShape.setPosition(*position);
        window_.draw(vehicleShape);
    }
}

void SimulationVisualizer::drawStatusPanel()
{
    constexpr float panelMargin{16.0F};
    constexpr float textHorizontalPadding{16.0F};
    constexpr float textTopPadding{12.0F};
    constexpr float textBottomPadding{16.0F};

    const char *status = "RUNNING";

    if (simulation_.finished())
    {
        status = "FINISHED";
    }
    else if (paused_)
    {
        status = "PAUSED";
    }

    std::ostringstream statusText;
    statusText << std::fixed << std::setprecision(1) << "TRAFFICSIM\n"
               << "Status: " << status << '\n'
               << "Time: " << simulation_.clock().currentTimeSeconds() << " / "
               << simulation_.config().durationSeconds << " s\n"
               << "Active vehicles: " << simulation_.vehicleManager().vehicleCount() << '\n'
               << "Spawned: " << simulation_.totalSpawnedVehicles() << '\n'
               << "Arrived: " << simulation_.totalArrivedVehicles() << '\n'
               << "Reroutes: " << simulation_.dynamicRoutingManager().totalReroutes() << "\n\n"
               << "CONTROLS\n"
               << "Space  Play / pause\n"
               << "Right  Single step\n"
               << "R      Reset\n"
               << "WASD   Pan camera\n"
               << "Wheel  Zoom\n"
               << "F      Fit network";

    sf::Text text{
        font_,
        statusText.str(),
        config_.statusCharacterSize,
    };
    text.setPosition({
        panelMargin + textHorizontalPadding,
        panelMargin + textTopPadding,
    });
    text.setFillColor(sf::Color{226U, 232U, 240U});

    const auto textBounds = text.getLocalBounds();
    const auto panelHeight =
        textTopPadding + textBounds.position.y + textBounds.size.y + textBottomPadding;

    window_.setView(window_.getDefaultView());

    sf::RectangleShape panel{
        sf::Vector2f{config_.statusPanelWidth, panelHeight},
    };
    panel.setPosition({panelMargin, panelMargin});
    panel.setFillColor(sf::Color{10U, 13U, 19U, 225U});
    panel.setOutlineColor(sf::Color{65U, 75U, 88U});
    panel.setOutlineThickness(1.0F);

    window_.draw(panel);
    window_.draw(text);
}

} // namespace trafficsim::visualization