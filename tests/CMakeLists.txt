# Engine tests. These link only against the engine and its JSON layer, so they
# build and run on a headless machine with no JUCE present - configure with
# -DGUITAR_BUILD_APP=OFF to do exactly that.

add_executable(guitar_core_tests
    TestMain.cpp
    PitchTests.cpp
    TuningTests.cpp
    FretboardTests.cpp
    ChordSymbolTests.cpp
    ChordShapesTests.cpp
    ShapeIdentifierTests.cpp
    ProgressTests.cpp
    FretboardQuizTests.cpp
    EngineApiTests.cpp)

target_link_libraries(guitar_core_tests PRIVATE guitar::core_engine guitar::engine_api)
target_include_directories(guitar_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(guitar_core_tests PRIVATE /W4)
else()
    target_compile_options(guitar_core_tests PRIVATE -Wall -Wextra)
endif()

add_test(NAME core_engine COMMAND guitar_core_tests)
