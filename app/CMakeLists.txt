# Platform Shell: the window, the webview and the app's lifecycle.
#
# This is the only target that knows about platforms. It is a JUCE "GUI
# Application" (standalone) target - a future plugin target would be a sibling of
# this directory reusing guitar::core_engine, which is why no engine code lives
# here.

juce_add_gui_app(GuitarLearningApp
    PRODUCT_NAME "Guitar Learning App"
    COMPANY_NAME "Guitar Learning App"
    BUNDLE_ID com.guitarlearningapp.poc
    VERSION ${PROJECT_VERSION}

    # Neither permission is wanted here. A guitar learner has no MIDI keyboard
    # to pair with, and listening to the instrument would be pitch detection -
    # deliberately out of scope, and a much larger undertaking than it sounds.
    MICROPHONE_PERMISSION_ENABLED FALSE
    BLUETOOTH_PERMISSION_ENABLED FALSE

    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait
                               UIInterfaceOrientationLandscapeLeft
                               UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS   UIInterfaceOrientationPortrait
                               UIInterfaceOrientationLandscapeLeft
                               UIInterfaceOrientationLandscapeRight)

# The page is the UI, so it ships inside the binary: no files beside the app,
# and no local server to start.
juce_add_binary_data(GuitarWebUiData
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../web/index.html)

target_sources(GuitarLearningApp PRIVATE
    src/Main.cpp
    src/WebUi.cpp)

target_include_directories(GuitarLearningApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(GuitarLearningApp PRIVATE
    GuitarWebUiData
    guitar::engine_api
    guitar::core_engine
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

target_compile_definitions(GuitarLearningApp PRIVATE
    # The UI is a web page in a webview, so this one is not optional.
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:GuitarLearningApp,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:GuitarLearningApp,JUCE_VERSION>")

# On Linux, JUCE's webview is WebKitGTK and JUCE leaves the flags to the project.
# macOS and Windows need nothing here: WKWebView is part of the system, and
# WebView2 comes in through JUCE_USE_WIN_WEBVIEW2 above.
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)

    # 4.1 is what current distributions ship; 4.0 is kept for older ones.
    pkg_check_modules(GUITAR_WEBKIT QUIET webkit2gtk-4.1)

    if(NOT GUITAR_WEBKIT_FOUND)
        pkg_check_modules(GUITAR_WEBKIT QUIET webkit2gtk-4.0)
    endif()

    if(NOT GUITAR_WEBKIT_FOUND)
        message(FATAL_ERROR
            "The app's UI is a web page in a webview, which on Linux needs WebKitGTK.\n"
            "Install libwebkit2gtk-4.1-dev (or libwebkit2gtk-4.0-dev) and libgtk-3-dev.")
    endif()

    target_include_directories(GuitarLearningApp PRIVATE ${GUITAR_WEBKIT_INCLUDE_DIRS})
    target_link_libraries(GuitarLearningApp PRIVATE ${GUITAR_WEBKIT_LIBRARIES})
    target_compile_options(GuitarLearningApp PRIVATE ${GUITAR_WEBKIT_CFLAGS_OTHER})
endif()
