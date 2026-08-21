cmake_minimum_required(VERSION 3.25)

project(TrafficSim
	VERSION 0.1.0
	DESCRIPTION "A deterministic traffic simulation engine written in modern C++."
	LANGUAGES CXX
)

include(FetchContent)

FetchContent_Declare(
	nlohmann_json
	URL
	https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
	URL_HASH
	SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
	DOWNLOAD_EXTRACT_TIMESTAMP
	TRUE
)

FetchContent_MakeAvailable(nlohmann_json)

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/scenarios")

configure_file(
	"${PROJECT_SOURCE_DIR}/scenarios/basic.json"
	"${PROJECT_BINARY_DIR}/scenarios/basic.json"
	COPYONLY
)

function(trafficsim_set_warnings targetName)
	if(MSVC)
		target_compile_options(${targetName} PRIVATE /W4 /permissive-)
	else()
		target_compile_options(${targetName} PRIVATE -Wall -Wextra -Wpedantic)
	endif()
endfunction()

add_library(trafficsim_core
	src/core/RandomGenerator.cpp
	src/core/Simulation.cpp
	src/core/SimulationClock.cpp
	src/core/SimulationConfig.cpp
	src/io/ScenarioLoader.cpp
	src/network/Intersection.cpp
	src/network/Road.cpp
	src/network/RoadNetwork.cpp
	src/routing/DijkstraRoutePlanner.cpp
	src/routing/Route.cpp
	src/statistics/ConsoleReporter.cpp
	src/statistics/CsvExporter.cpp
	src/statistics/StatisticsCollector.cpp
	src/traffic/CongestionClassifier.cpp
	src/traffic/RoadTrafficMonitor.cpp
	src/traffic/TrafficLight.cpp
	src/traffic/TrafficLightController.cpp
	src/traffic/TrafficManager.cpp
	src/vehicles/Vehicle.cpp
	src/vehicles/VehicleManager.cpp
	src/vehicles/VehicleSpawner.cpp
)

target_include_directories(trafficsim_core
	PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(trafficsim_core
	PRIVATE nlohmann_json::nlohmann_json
)

target_compile_features(trafficsim_core PUBLIC cxx_std_20)

set_target_properties(trafficsim_core
	PROPERTIES CXX_EXTENSIONS OFF
)

trafficsim_set_warnings(trafficsim_core)

add_executable(trafficsim
	src/app/main.cpp
)

target_link_libraries(trafficsim PRIVATE trafficsim_core)
trafficsim_set_warnings(trafficsim)

include(CTest)

if(BUILD_TESTING)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

	FetchContent_Declare(
		googletest
		URL
		https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
		DOWNLOAD_EXTRACT_TIMESTAMP
		TRUE
	)

	FetchContent_MakeAvailable(googletest)

	add_executable(trafficsim_tests
		tests/integration/ScenarioSimulationTests.cpp
		tests/unit/CongestionClassifierTests.cpp
		tests/unit/ConsoleReporterTests.cpp
		tests/unit/CsvExporterTests.cpp
		tests/unit/DijkstraRoutePlannerTests.cpp
		tests/unit/IntersectionTests.cpp
		tests/unit/ProjectSetupTests.cpp
		tests/unit/RandomGeneratorTests.cpp
		tests/unit/RoadNetworkTests.cpp
		tests/unit/RoadTests.cpp
		tests/unit/RoadTrafficMonitorTests.cpp
		tests/unit/RouteTests.cpp
		tests/unit/ScenarioLoaderTests.cpp
		tests/unit/SimulationClockTests.cpp
		tests/unit/SimulationConfigTests.cpp
		tests/unit/SimulationTests.cpp
		tests/unit/StatisticsCollectorTests.cpp
		tests/unit/TrafficLightControllerTests.cpp
		tests/unit/TrafficLightTests.cpp
		tests/unit/TrafficManagerTests.cpp
		tests/unit/VehicleFollowingTests.cpp
		tests/unit/VehicleManagerTests.cpp
		tests/unit/VehicleMovementTests.cpp
		tests/unit/VehicleSpawnerTests.cpp
		tests/unit/VehicleTests.cpp
		tests/unit/VehicleTrafficLightTests.cpp
	)

	target_link_libraries(trafficsim_tests
		PRIVATE
			trafficsim_core
			GTest::gtest_main
	)

	trafficsim_set_warnings(trafficsim_tests)

	include(GoogleTest)
	gtest_discover_tests(trafficsim_tests)
endif()
