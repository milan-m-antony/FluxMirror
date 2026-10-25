cmake_minimum_required(VERSION 3.22)

project(MirrorApp
    VERSION 1.0.0
    DESCRIPTION "High-Performance Native Cross-Platform Android Screen Mirror & Controller"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTING "Build unit tests" ON)

# Find Qt 6
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    Qml
    Network
    Concurrent
)

qt_standard_project_setup(REQUIRES 6.4)

# Find FFmpeg (avcodec, avutil, swscale)
find_package(PkgConfig QUIET)
find_package(FFmpeg COMPONENTS AVCODEC AVUTIL SWSCALE QUIET)

if(NOT FFmpeg_FOUND)
    # vcpkg fallback / manual include detection
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
    find_library(AVCODEC_LIBRARY avcodec)
    find_library(AVUTIL_LIBRARY avutil)
    find_library(SWSCALE_LIBRARY swscale)
endif()

# Source files
set(SOURCES
    src/app/main.cpp
    src/app/AppController.cpp
    src/device/DeviceManager.cpp
    src/connection/ConnectionManager.cpp
    src/mirroring/MirrorEngine.cpp
    src/video/VideoDecoder.cpp
    src/rendering/VideoRenderer.cpp
    src/input/InputController.cpp
    src/clipboard/ClipboardManager.cpp
    src/window/WindowManager.cpp
    src/diagnostics/PerformanceMonitor.cpp
)

set(HEADERS
    src/app/AppController.hpp
    src/device/DeviceManager.hpp
    src/connection/ConnectionManager.hpp
    src/mirroring/MirrorEngine.hpp
    src/mirroring/ScrcpyProtocol.hpp
    src/video/VideoDecoder.hpp
    src/rendering/VideoRenderer.hpp
    src/input/InputController.hpp
    src/clipboard/ClipboardManager.hpp
    src/window/WindowManager.hpp
    src/diagnostics/PerformanceMonitor.hpp
)

# QML Resources
qt_add_qml_module(MirrorApp
    URI MirrorApp
    VERSION 1.0
    SOURCES
        ${SOURCES}
        ${HEADERS}
    QML_FILES
        src/ui/Main.qml
        src/ui/MirrorViewport.qml
        src/ui/DeviceToolbar.qml
        src/ui/PerformanceOverlay.qml
    RESOURCES
        resources/icons/camera.svg
        resources/icons/rotate.svg
        resources/icons/pin.svg
        resources/icons/fullscreen.svg
)

# Include directories
target_include_directories(MirrorApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(AVCODEC_INCLUDE_DIR)
    target_include_directories(MirrorApp PRIVATE ${AVCODEC_INCLUDE_DIR})
endif()

# Link Libraries
target_link_libraries(MirrorApp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Concurrent
)

if(TARGET FFmpeg::avcodec)
    target_link_libraries(MirrorApp PRIVATE FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale)
elseif(AVCODEC_LIBRARY)
    target_link_libraries(MirrorApp PRIVATE ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
endif()

# Platform specific configurations
if(WIN32)
    target_compile_definitions(MirrorApp PRIVATE PLATFORM_WINDOWS OS_WINDOWS)
    target_link_libraries(MirrorApp PRIVATE user32 shell32)
elseif(APPLE)
    target_compile_definitions(MirrorApp PRIVATE PLATFORM_MACOS OS_MACOS)
elseif(UNIX)
    target_compile_definitions(MirrorApp PRIVATE PLATFORM_LINUX OS_LINUX)
endif()

# Unit tests
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
