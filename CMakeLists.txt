cmake_minimum_required(VERSION 3.22)

project(GuitarLearningApp
        VERSION 0.1.0
        DESCRIPTION "Guitar learning app - chord voicings and the fretboard"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)              # JUCE 8 requires C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(GUITAR_BUILD_APP   "Build the JUCE standalone application (requires JUCE)" ON)
option(GUITAR_BUILD_TESTS "Build the core engine unit tests" ON)

# The core engine is plain C++ with no JUCE dependency, so it always builds -
# including on a machine that has no JUCE and no GUI toolkit installed. That is
# the architectural boundary this project is built around, enforced by the build
# rather than by code review.
add_subdirectory(modules/core_engine)
add_subdirectory(modules/engine_api)

if(GUITAR_BUILD_APP)
    include(cmake/GetJUCE.cmake)
    add_subdirectory(app)
endif()

if(GUITAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "Guitar Learning App ${PROJECT_VERSION}")
message(STATUS "  Standalone app : ${GUITAR_BUILD_APP}")
message(STATUS "  Engine tests   : ${GUITAR_BUILD_TESTS}")
