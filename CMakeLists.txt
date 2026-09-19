cmake_minimum_required(VERSION 3.21)
project(bloonstd4repopped VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_GAME "Build desktop/console game executable" ON)
option(BUILD_BUILDER "Build Dear ImGui Game Builder desktop app" ON)
option(BUILD_TESTS "Build automated unit tests" ON)
option(TARGET_PSP "Target Sony PlayStation Portable" OFF)

if(PSP OR PLATFORM_PSP)
    set(TARGET_PSP ON)
endif()
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

if(NOT TARGET_PSP)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.16
        GIT_SHALLOW TRUE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3)

    FetchContent_Declare(SDL3_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG release-3.4.4
        GIT_SHALLOW TRUE)
    set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_GIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_LBM OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_PCX OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_PNM OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_SVG OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TGA OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_XCF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_XPM OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_XV OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3_image)

    FetchContent_Declare(SDL3_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG release-3.2.4
        GIT_SHALLOW TRUE)
    set(SDLMIXER_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_FLAC OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MIDI OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MP3 ON CACHE BOOL "" FORCE)
    set(SDLMIXER_MP3_DRMP3 ON CACHE BOOL "" FORCE)
    set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VORBIS_STB OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3_mixer)

    if(BUILD_BUILDER)
        FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.92.9b
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(imgui)
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
        target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    endif()
endif()

add_library(btd4_engine STATIC
    engine/core/Logger.hpp engine/core/Logger.cpp engine/core/Clock.hpp engine/core/Clock.cpp
    engine/core/IFileSystem.hpp engine/audio/IAudio.hpp engine/core/Engine.hpp engine/core/Engine.cpp
    engine/rendering/IRenderer.hpp engine/rendering/LogicalResolution.hpp engine/rendering/LogicalResolution.cpp
    engine/rendering/TestScreen.hpp engine/rendering/TestScreen.cpp engine/input/IInput.hpp
    engine/input/InputActions.hpp engine/input/FrontendProfile.hpp engine/map/Map.hpp engine/map/Map.cpp
    engine/map/MapLoader.cpp engine/map/TrackEditor.hpp engine/map/TrackEditor.cpp
    engine/game/Bloon.hpp engine/game/Bloon.cpp engine/game/Projectile.hpp
    engine/game/Projectile.cpp engine/game/Tower.hpp engine/game/Tower.cpp engine/game/TowerData.hpp
    engine/game/TowerData.cpp engine/game/Upgrade.hpp engine/game/Player.hpp engine/game/Player.cpp
    engine/game/MultiplayerSession.hpp engine/game/MultiplayerSession.cpp
    engine/game/Upgrade.cpp engine/game/Economy.hpp engine/game/Economy.cpp engine/game/GameState.hpp
    engine/game/GameState.cpp engine/game/Rounds.cpp engine/game/RoundLoader.cpp engine/game/Freeplay.hpp
    engine/game/Freeplay.cpp engine/rendering/TextureAtlas.hpp engine/rendering/TextureAtlas.cpp
    engine/rendering/SpriteBatch.hpp engine/rendering/SpriteBatch.cpp engine/rendering/Camera.hpp
    engine/rendering/Camera.cpp engine/rendering/DisplayConfig.hpp engine/rendering/DisplayConfig.cpp
    engine/rendering/DebugRenderer.hpp engine/rendering/DebugRenderer.cpp engine/assets/AssetManifest.hpp
    engine/assets/AssetManifest.cpp engine/assets/AssetManager.hpp engine/assets/AssetManager.cpp)
target_include_directories(btd4_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine ${CMAKE_CURRENT_SOURCE_DIR}/platform/common)

if(TARGET_PSP)
    set(PLATFORM_SOURCES platform/psp/PSPLifecycle.cpp)
else()
    set(PLATFORM_SOURCES
        platform/common/PlatformBackend.hpp platform/common/PlatformRegistry.hpp platform/common/PlatformRegistry.cpp
        platform/common/NullAudio.hpp platform/common/NullAudio.cpp platform/linux/LinuxPlatform.hpp
        platform/linux/LinuxPlatform.cpp platform/windows/WindowsPlatform.hpp platform/windows/WindowsPlatform.cpp
        platform/psp/PSPBackend.hpp platform/psp/PSPBackend.cpp platform/xbox360/Xbox360Backend.hpp
        platform/xbox360/Xbox360Backend.cpp platform/future/FuturePlatformBackend.hpp
        platform/future/FuturePlatformBackend.cpp platform/common/NativeFileSystem.hpp platform/common/NativeFileSystem.cpp
        platform/common/SDLRenderer.hpp platform/common/SDLRenderer.cpp platform/common/SDLInput.hpp platform/common/SDLInput.cpp
        platform/common/SDLAudio.hpp platform/common/SDLAudio.cpp)
endif()
add_library(btd4_platform STATIC ${PLATFORM_SOURCES})
target_include_directories(btd4_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/platform/common ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows ${CMAKE_CURRENT_SOURCE_DIR}/platform/psp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/xbox360 ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_link_libraries(btd4_platform PUBLIC btd4_engine)
if(NOT TARGET_PSP)
    target_link_libraries(btd4_platform PUBLIC SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer)
endif()

if(BUILD_GAME)
    if(NOT TARGET_PSP)
        add_executable(btd4_game app/main.cpp)
        target_link_libraries(btd4_game PRIVATE btd4_engine btd4_platform SDL3::SDL3)
        if(WIN32)
            add_custom_command(TARGET btd4_game POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:btd4_game>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:SDL3_image::SDL3_image>" "$<TARGET_FILE_DIR:btd4_game>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:SDL3_mixer::SDL3_mixer>" "$<TARGET_FILE_DIR:btd4_game>"
                COMMENT "Deploying SDL3 desktop runtimes for btd4_game")
        endif()
    else()
        if(NOT COMMAND create_pbp_file)
            message(FATAL_ERROR "PSP builds require PSPSDK's CMake toolchain (use psp-cmake or the psp-release preset).")
        endif()
        add_executable(btd4_game platform/psp/main.cpp)
        target_link_libraries(btd4_game PRIVATE btd4_platform pspdebug pspdisplay pspge pspctrl)
        create_pbp_file(TARGET btd4_game TITLE "Bloons TD 4 Repopped" VERSION "01.00" BUILD_PRX)
        add_custom_command(TARGET btd4_game POST_BUILD COMMAND ${CMAKE_COMMAND} -DPBP_FILE=${CMAKE_CURRENT_BINARY_DIR}/EBOOT.PBP -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyPBP.cmake
            BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/EBOOT.PBP ${CMAKE_CURRENT_BINARY_DIR}/btd4_game.prx VERBATIM)
        enable_testing()
        add_test(NAME psp_package_structure COMMAND ${CMAKE_COMMAND} -DPBP_FILE=${CMAKE_CURRENT_BINARY_DIR}/EBOOT.PBP -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyPBP.cmake)
    endif()
endif()

if(NOT TARGET_PSP)
    add_library(btd4_importer_lib STATIC
        tools/swf/SwfTypes.hpp tools/swf/SwfReader.hpp tools/swf/SwfReader.cpp tools/swf/Inflate.hpp tools/swf/Inflate.cpp
        tools/swf/SwfParser.hpp tools/swf/SwfParser.cpp tools/asset_converter/AssetConverter.hpp tools/asset_converter/AssetConverter.cpp
        tools/archive/ZipArchive.hpp tools/archive/ZipArchive.cpp tools/asset_importer/AssetImporter.hpp tools/asset_importer/AssetImporter.cpp)
    target_include_directories(btd4_importer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tools ${CMAKE_CURRENT_SOURCE_DIR}/engine)
    add_executable(btd4_importer tools/asset_importer/main.cpp)
    target_link_libraries(btd4_importer PRIVATE btd4_importer_lib)
endif()

if(BUILD_BUILDER AND NOT TARGET_PSP)
    # The builder is a GUI application on Windows. Use the WIN32 subsystem so
    # double-clicking it does not spawn a persistent console window.
    if(WIN32)
        add_executable(btd4_builder WIN32
            builder/main.cpp builder/project/Project.hpp builder/project/Project.cpp
            builder/ui/BuilderUI.hpp builder/ui/BuilderUI.cpp
            builder/map/MapEditor.hpp builder/map/MapEditor.cpp
            builder/round/RoundEditor.hpp builder/round/RoundEditor.cpp
            builder/tower/TowerEditor.hpp builder/tower/TowerEditor.cpp
            builder/upgrade/UpgradeEditor.hpp builder/upgrade/UpgradeEditor.cpp
            builder/platform/FileDialog.hpp builder/platform/FileDialog.cpp)
    else()
        add_executable(btd4_builder
            builder/main.cpp builder/project/Project.hpp builder/project/Project.cpp
            builder/ui/BuilderUI.hpp builder/ui/BuilderUI.cpp
            builder/map/MapEditor.hpp builder/map/MapEditor.cpp
            builder/round/RoundEditor.hpp builder/round/RoundEditor.cpp
            builder/tower/TowerEditor.hpp builder/tower/TowerEditor.cpp
            builder/upgrade/UpgradeEditor.hpp builder/upgrade/UpgradeEditor.cpp
            builder/platform/FileDialog.hpp builder/platform/FileDialog.cpp)
    endif()
    target_include_directories(btd4_builder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/builder ${CMAKE_CURRENT_SOURCE_DIR}/tools
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/common ${CMAKE_CURRENT_SOURCE_DIR}/engine)
    target_link_libraries(btd4_builder PRIVATE btd4_platform btd4_engine imgui SDL3::SDL3)
    if(WIN32)
        target_link_libraries(btd4_builder PRIVATE comdlg32)
        add_custom_command(TARGET btd4_builder POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:btd4_builder>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3_image::SDL3_image>" "$<TARGET_FILE_DIR:btd4_builder>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3_mixer::SDL3_mixer>" "$<TARGET_FILE_DIR:btd4_builder>"
            COMMENT "Deploying SDL3 desktop runtimes for btd4_builder")
        if(TARGET btd4_importer)
            add_custom_command(TARGET btd4_builder POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:btd4_importer>" "$<TARGET_FILE_DIR:btd4_builder>"
                COMMENT "Deploying standalone asset importer next to btd4_builder")
        endif()
    elseif(TARGET btd4_importer)
        add_custom_command(TARGET btd4_builder POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:btd4_importer>" "$<TARGET_FILE_DIR:btd4_builder>"
            COMMENT "Deploying standalone asset importer next to btd4_builder")
    endif()
endif()

if(BUILD_TESTS AND NOT TARGET_PSP)
    enable_testing()
    add_executable(btd4_tests tests/main.cpp tests/MapLoaderTests.cpp tests/RoundTests.cpp tests/AssetImporterTests.cpp tests/SwfSpriteTests.cpp tests/SwfLosslessTests.cpp
        tests/RenderingTests.cpp tests/FreeplayTests.cpp tests/UpgradeTests.cpp tests/TestRunner.hpp
        builder/project/Project.hpp builder/project/Project.cpp)
    target_include_directories(btd4_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/builder
        ${CMAKE_CURRENT_SOURCE_DIR}/tools ${CMAKE_CURRENT_SOURCE_DIR}/engine ${CMAKE_CURRENT_SOURCE_DIR}/platform/common)
    target_link_libraries(btd4_tests PRIVATE btd4_engine btd4_platform btd4_importer_lib SDL3::SDL3)
    if(WIN32)
        target_link_options(btd4_tests PRIVATE "/SUBSYSTEM:CONSOLE")
        add_custom_command(TARGET btd4_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:btd4_tests>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3_image::SDL3_image>" "$<TARGET_FILE_DIR:btd4_tests>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:SDL3_mixer::SDL3_mixer>" "$<TARGET_FILE_DIR:btd4_tests>"
            COMMENT "Deploying SDL3 runtimes for btd4_tests")
    endif()
    add_test(NAME btd4_unit_tests COMMAND btd4_tests)
    set_tests_properties(btd4_unit_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
