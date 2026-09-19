# Core Engine: the fretboard, chord parsing, voicing generation, shape reading
# and the quiz. Pure C++17 - no JUCE, no GUI, no platform APIs.
#
# Do not add a JUCE dependency to this target. Keeping it free of JUCE is what
# lets the same engine back a future plugin or mobile target, and what lets the
# unit tests run anywhere.

add_library(guitar_core_engine STATIC
    src/Pitch.cpp
    src/Tuning.cpp
    src/Fretboard.cpp
    src/ChordSymbol.cpp
    src/ChordShapes.cpp
    src/ShapeIdentifier.cpp
    src/FretboardQuiz.cpp
    src/Progress.cpp)

add_library(guitar::core_engine ALIAS guitar_core_engine)

target_include_directories(guitar_core_engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(guitar_core_engine PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(guitar_core_engine PRIVATE /W4)
else()
    target_compile_options(guitar_core_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
