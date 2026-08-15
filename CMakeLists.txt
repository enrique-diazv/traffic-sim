cmake_minimum_required(VERSION 3.25)

project(
    TrafficSim
    VERSION 0.1.0
    DESCRIPTION "A deterministic traffic simulation engine written in modern C++."
    LANGUAGES CXX
)

function(trafficsim_set_warnings targetName)
    if(MSVC)
        target_compile_options(${targetName} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${targetName} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_executable(
    trafficsim
    src/app/main.cpp
)

target_compile_features(trafficsim PRIVATE cxx_std_20)

set_target_properties(
    trafficsim
    PROPERTIES
        CXX_EXTENSIONS OFF
)

trafficsim_set_warnings(trafficsim)

include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(
        trafficsim_tests
        tests/unit/ProjectSetupTests.cpp
    )

    target_compile_features(trafficsim_tests PRIVATE cxx_std_20)
    target_link_libraries(trafficsim_tests PRIVATE GTest::gtest_main)

    set_target_properties(
        trafficsim_tests
        PROPERTIES
            CXX_EXTENSIONS OFF
    )

    trafficsim_set_warnings(trafficsim_tests)

    include(GoogleTest)
    gtest_discover_tests(trafficsim_tests)
endif()