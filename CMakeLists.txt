cmake_minimum_required(VERSION 3.28)

project(TrafficSim
	VERSION 0.1.0
	DESCRIPTION "A deterministic traffic simulation engine written in modern C++."
	LANGUAGES CXX
)

include(FetchContent)
find_package(Threads REQUIRED)

option(TRAFFICSIM_BUILD_VISUALIZER
	"Build the optional SFML visualization frontend."
	OFF
)

option(TRAFFICSIM_BUILD_BENCHMARKS
	"Build the optional performance benchmark executable."
	OFF
)

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

if(TRAFFICSIM_BUILD_VISUALIZER)
	set(
		SFML_BUILD_AUDIO
		OFF
		CACHE
			BOOL
			"Do not build the unused SFML Audio module."
		FORCE
	)
	set(
		SFML_BUILD_NETWORK
		OFF
		CACHE
			BOOL
			"Do not build the unused SFML Network module."
		FORCE
	)

	FetchContent_Declare(
		SFML
		GIT_REPOSITORY
		https://github.com/SFML/SFML.git
		GIT_TAG
		3.1.0
		GIT_SHALLOW
		ON
		EXCLUDE_FROM_ALL
		SYSTEM
	)

	FetchContent_MakeAvailable(SFML)
endif()

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/scenarios")

configure_file(
	"${PROJECT_SOURCE_DIR}/scenarios/basic.json"
	"${PROJECT_BINARY_DIR}/scenarios/basic.json"
	COPYONLY
)

configure_file(
	"${PROJECT_SOURCE_DIR}/scenarios/alternate_routes.json"
	"${PROJECT_BINARY_DIR}/scenarios/alternate_routes.json"
	COPYONLY
)

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/experiments")

configure_file(
	"${PROJECT_SOURCE_DIR}/experiments/basic_sweep.json"
	"${PROJECT_BINARY_DIR}/experiments/basic_sweep.json"
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
	src/experiments/BatchExperimentConfig.cpp
	src/experiments/BatchExperimentRunner.cpp
	src/experiments/ExperimentAggregator.cpp
	src/experiments/ExperimentCsvExporter.cpp
	src/experiments/ExperimentPlan.cpp
	src/experiments/ExperimentPlanLoader.cpp
	src/experiments/ParameterSweep.cpp
	src/io/ScenarioLoader.cpp
	src/network/Intersection.cpp
	src/network/Road.cpp
	src/network/RoadNetwork.cpp
	src/routing/AStarRoutePlanner.cpp
	src/routing/CongestionAwareRouteCost.cpp
	src/routing/DijkstraRoutePlanner.cpp
	src/routing/DynamicRoutingManager.cpp
	src/routing/ReroutingPolicy.cpp
	src/routing/Route.cpp
	src/routing/RouteComparator.cpp
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
	PRIVATE
		nlohmann_json::nlohmann_json
		Threads::Threads
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

add_executable(trafficsim_experiments
	src/experiment_app/main.cpp
)

target_link_libraries(trafficsim_experiments PRIVATE trafficsim_core)
trafficsim_set_warnings(trafficsim_experiments)

if(TRAFFICSIM_BUILD_BENCHMARKS)
	add_executable(trafficsim_benchmarks
		benchmarks/BenchmarkCsvExporter.cpp
		benchmarks/BenchmarkCsvExporter.h
		benchmarks/BenchmarkFixtures.cpp
		benchmarks/BenchmarkFixtures.h
		benchmarks/BenchmarkTimer.h
		benchmarks/BenchmarkTypes.cpp
		benchmarks/BenchmarkTypes.h
		benchmarks/PerformanceBenchmarkRunner.cpp
		benchmarks/PerformanceBenchmarkRunner.h
		benchmarks/ProcessCpuTime.cpp
		benchmarks/ProcessCpuTime.h
		benchmarks/main.cpp
	)

	target_include_directories(trafficsim_benchmarks
		PRIVATE ${PROJECT_SOURCE_DIR}/benchmarks
	)

	target_link_libraries(trafficsim_benchmarks
		PRIVATE trafficsim_core
	)

	trafficsim_set_warnings(trafficsim_benchmarks)
endif()

if(TRAFFICSIM_BUILD_VISUALIZER)
	add_executable(trafficsim_visualizer
		src/visualization/SimulationVisualizer.cpp
		src/visualization/SimulationVisualizer.h
		src/visualization/main.cpp
	)

	target_link_libraries(trafficsim_visualizer
		PRIVATE
			trafficsim_core
			SFML::Graphics
	)

	trafficsim_set_warnings(trafficsim_visualizer)
	add_custom_command(
		TARGET trafficsim_visualizer
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory
			"$<TARGET_FILE_DIR:trafficsim_visualizer>/assets/fonts"
		COMMAND ${CMAKE_COMMAND} -E make_directory
			"$<TARGET_FILE_DIR:trafficsim_visualizer>/scenarios"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${PROJECT_SOURCE_DIR}/assets/fonts/RobotoMono-Regular.ttf"
			"$<TARGET_FILE_DIR:trafficsim_visualizer>/assets/fonts/RobotoMono-Regular.ttf"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${PROJECT_SOURCE_DIR}/assets/fonts/OFL.txt"
			"$<TARGET_FILE_DIR:trafficsim_visualizer>/assets/fonts/OFL.txt"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${PROJECT_SOURCE_DIR}/scenarios/basic.json"
			"$<TARGET_FILE_DIR:trafficsim_visualizer>/scenarios/basic.json"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${PROJECT_SOURCE_DIR}/scenarios/alternate_routes.json"
			"$<TARGET_FILE_DIR:trafficsim_visualizer>/scenarios/alternate_routes.json"
		VERBATIM
	)
endif()

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
		tests/integration/BatchExperimentPipelineTests.cpp
		tests/integration/ScenarioSimulationTests.cpp
		tests/unit/AStarRoutePlannerTests.cpp
		tests/unit/BatchExperimentConfigTests.cpp
		tests/unit/BatchExperimentRunnerTests.cpp
		tests/unit/CongestionAwareRouteCostTests.cpp
		tests/unit/CongestionClassifierTests.cpp
		tests/unit/ConsoleReporterTests.cpp
		tests/unit/CsvExporterTests.cpp
		tests/unit/DijkstraRoutePlannerTests.cpp
		tests/unit/DynamicRoutingManagerTests.cpp
		tests/unit/ExperimentAggregatorTests.cpp
		tests/unit/ExperimentCsvExporterTests.cpp
		tests/unit/ExperimentPlanTests.cpp
		tests/unit/ExperimentPlanLoaderTests.cpp
		tests/unit/IntersectionTests.cpp
		tests/unit/ParameterSweepTests.cpp
		tests/unit/ProjectSetupTests.cpp
		tests/unit/RandomGeneratorTests.cpp
		tests/unit/ReroutingPolicyTests.cpp
		tests/unit/RoadNetworkTests.cpp
		tests/unit/RoadTests.cpp
		tests/unit/RoadTrafficMonitorTests.cpp
		tests/unit/RouteTests.cpp
		tests/unit/RouteComparatorTests.cpp
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
	gtest_discover_tests(
		trafficsim_tests
		DISCOVERY_TIMEOUT
		30
	)
endif()
