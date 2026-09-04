cmake_minimum_required(VERSION 3.20)
project(bloonstd4repopped VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_GAME "Build desktop/console game executable" ON)
option(BUILD_BUILDER "Build Dear ImGui Game Builder desktop app" ON)
option(BUILD_TESTS "Build automated unit tests" ON)
option(TARGET_PSP "Target Sony PlayStation Portable" OFF)

if(TARGET_PSP OR DEFINED CMAKE_TOOLCHAIN_FILE)
    if(CMAKE_TOOLCHAIN_FILE MATCHES "psp")
        set(TARGET_PSP ON)
    endif()
endif()

if(TARGET_PSP)
    set(BUILD_BUILDER OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_compile_definitions(__PSP__)
endif()

include(FetchContent)

# -----------------------------------------------------------------------------
# Dependencies (Desktop targets)
# -----------------------------------------------------------------------------
if(NOT TARGET_PSP)
    # SDL2
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        # Try pkg-config
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
        endif()
    endif()

    if(NOT SDL2_FOUND AND NOT TARGET SDL2::SDL2)
        message(STATUS "SDL2 not found on host. Fetching SDL2 via FetchContent...")
        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-2.30.8
            GIT_SHALLOW TRUE
        )
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2)
        if(TARGET SDL2-shared)
            add_library(SDL2::SDL2 ALIAS SDL2-shared)
        endif()
    endif()

    # Dear ImGui (for Game Builder)
    if(BUILD_BUILDER)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.91.5
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(imgui)

        # ImGui Library Target
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
        )
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        if(TARGET SDL2::SDL2)
            target_link_libraries(imgui PUBLIC SDL2::SDL2)
        elseif(SDL2_LIBRARIES)
            target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES})
            target_include_directories(imgui PUBLIC ${SDL2_INCLUDE_DIRS})
        endif()
    endif()
endif()

# -----------------------------------------------------------------------------
# Target: btd4_engine
# -----------------------------------------------------------------------------
add_library(btd4_engine STATIC
    engine/core/Logger.hpp
    engine/core/Logger.cpp
    engine/core/Clock.hpp
    engine/core/Clock.cpp
    engine/core/IFileSystem.hpp
    engine/audio/IAudio.hpp
    engine/core/Engine.hpp
    engine/core/Engine.cpp
    engine/rendering/IRenderer.hpp
    engine/rendering/LogicalResolution.hpp
    engine/rendering/LogicalResolution.cpp
    engine/rendering/TestScreen.hpp
    engine/rendering/TestScreen.cpp
    engine/input/IInput.hpp
    engine/input/InputActions.hpp
    engine/map/Map.hpp
    engine/map/Map.cpp
    engine/game/Bloon.hpp
    engine/game/Bloon.cpp
    engine/game/Projectile.hpp
    engine/game/Projectile.cpp
    engine/game/Tower.hpp
    engine/game/Tower.cpp
    engine/game/Economy.hpp
    engine/game/Economy.cpp
    engine/game/GameState.hpp
    engine/game/GameState.cpp
)

target_include_directories(btd4_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/common
)

# -----------------------------------------------------------------------------
# Target: btd4_platform
# -----------------------------------------------------------------------------
set(PLATFORM_SOURCES
    platform/common/PlatformBackend.hpp
    platform/common/PlatformRegistry.hpp
    platform/common/PlatformRegistry.cpp
    platform/common/NullAudio.hpp
    platform/common/NullAudio.cpp
    platform/linux/LinuxPlatform.hpp
    platform/linux/LinuxPlatform.cpp
    platform/windows/WindowsPlatform.hpp
    platform/windows/WindowsPlatform.cpp
    platform/psp/PSPBackend.hpp
    platform/psp/PSPBackend.cpp
    platform/xbox360/Xbox360Backend.hpp
    platform/xbox360/Xbox360Backend.cpp
)

if(NOT TARGET_PSP)
    list(APPEND PLATFORM_SOURCES
        platform/common/NativeFileSystem.hpp
        platform/common/NativeFileSystem.cpp
        platform/common/SDLRenderer.hpp
        platform/common/SDLRenderer.cpp
        platform/common/SDLInput.hpp
        platform/common/SDLInput.cpp
    )
endif()

add_library(btd4_platform STATIC ${PLATFORM_SOURCES})
target_include_directories(btd4_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/common
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/psp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/xbox360
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
)
target_link_libraries(btd4_platform PUBLIC btd4_engine)

if(NOT TARGET_PSP)
    if(TARGET SDL2::SDL2)
        target_link_libraries(btd4_platform PUBLIC SDL2::SDL2)
    elseif(SDL2_LIBRARIES)
        target_link_libraries(btd4_platform PUBLIC ${SDL2_LIBRARIES})
        target_include_directories(btd4_platform PUBLIC ${SDL2_INCLUDE_DIRS})
    endif()
endif()

# -----------------------------------------------------------------------------
# Target: btd4_game
# -----------------------------------------------------------------------------
if(BUILD_GAME)
    if(NOT TARGET_PSP)
        add_executable(btd4_game app/main.cpp)
        target_link_libraries(btd4_game PRIVATE btd4_engine btd4_platform)
        if(TARGET SDL2::SDL2)
            target_link_libraries(btd4_game PRIVATE SDL2::SDL2)
        elseif(SDL2_LIBRARIES)
            target_link_libraries(btd4_game PRIVATE ${SDL2_LIBRARIES})
        endif()
    else()
        # PSP Target
        add_executable(btd4_game app/main.cpp)
        target_link_libraries(btd4_game PRIVATE btd4_engine btd4_platform ${PSP_STANDARD_LIBS})
    endif()
endif()

# -----------------------------------------------------------------------------
# Target: btd4_builder
# -----------------------------------------------------------------------------
if(BUILD_BUILDER AND NOT TARGET_PSP)
    add_executable(btd4_builder
        builder/main.cpp
        builder/project/Project.hpp
        builder/project/Project.cpp
        builder/ui/BuilderUI.hpp
        builder/ui/BuilderUI.cpp
    )
    target_include_directories(btd4_builder PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/builder
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/common
        ${CMAKE_CURRENT_SOURCE_DIR}/engine
    )
    target_link_libraries(btd4_builder PRIVATE
        btd4_platform
        btd4_engine
        imgui
    )
    if(TARGET SDL2::SDL2)
        target_link_libraries(btd4_builder PRIVATE SDL2::SDL2)
    elseif(SDL2_LIBRARIES)
        target_link_libraries(btd4_builder PRIVATE ${SDL2_LIBRARIES})
    endif()
endif()

# -----------------------------------------------------------------------------
# Target: btd4_tests
# -----------------------------------------------------------------------------
if(BUILD_TESTS AND NOT TARGET_PSP)
    enable_testing()
    add_executable(btd4_tests
        tests/main.cpp
        tests/TestRunner.hpp
        builder/project/Project.hpp
        builder/project/Project.cpp
    )
    target_include_directories(btd4_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/builder
        ${CMAKE_CURRENT_SOURCE_DIR}/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/common
    )
    target_link_libraries(btd4_tests PRIVATE
        btd4_engine
        btd4_platform
    )
    target_compile_definitions(btd4_tests PRIVATE SDL_MAIN_HANDLED)
    if(TARGET SDL2::SDL2)
        target_link_libraries(btd4_tests PRIVATE SDL2::SDL2)
    elseif(SDL2_LIBRARIES)
        target_link_libraries(btd4_tests PRIVATE ${SDL2_LIBRARIES})
    endif()
    add_test(NAME btd4_unit_tests COMMAND btd4_tests)
endif()
