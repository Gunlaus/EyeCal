cmake_minimum_required(VERSION 3.24)
project(EyeCal VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(CTest)
include(FetchContent)
option(EYECAL_BUILD_APP "Build the Vulkan executable" ON)
if(NOT EYECAL_BUILD_APP AND NOT BUILD_TESTING)
    return()
endif()

FetchContent_Declare(vulkan_headers
    URL https://codeload.github.com/KhronosGroup/Vulkan-Headers/tar.gz/refs/tags/v1.4.321
    URL_HASH SHA256=7c1874f9186d9e7d5245e279cdccaa77a66ed8f52871b428630aeb771dc7623c)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nlohmann_json
    URL https://codeload.github.com/nlohmann/json/tar.gz/refs/tags/v3.12.0
    URL_HASH SHA256=4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187)
FetchContent_MakeAvailable(vulkan_headers nlohmann_json)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")
set(ENGLISH_LOCALE "${CMAKE_CURRENT_SOURCE_DIR}/assets/locales/en.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ENGLISH_LOCALE}")
file(READ "${ENGLISH_LOCALE}" english_json)
configure_file(cmake/locale.hpp.in "${GENERATED_DIR}/locale.hpp" @ONLY)

add_library(eyecal_core STATIC src/guide/guide.cpp src/patterns/quantize.cpp src/ui/interaction.cpp)
target_include_directories(eyecal_core PUBLIC src PRIVATE assets/blue-noise)
target_link_libraries(eyecal_core PRIVATE nlohmann_json::nlohmann_json)
if(MSVC)
    target_compile_options(eyecal_core PRIVATE /W4 /permissive- /fp:strict)
else()
    target_compile_options(eyecal_core PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
endif()

if(BUILD_TESTING)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(doctest
        URL https://codeload.github.com/doctest/doctest/tar.gz/refs/tags/v2.4.12
        URL_HASH SHA256=73381c7aa4dee704bd935609668cf41880ea7f19fa0504a200e13b74999c2d70)
    FetchContent_MakeAvailable(doctest)
    add_executable(surface_tests tests/surface_tests.cpp)
    target_include_directories(surface_tests PRIVATE src)
    target_link_libraries(surface_tests PRIVATE Vulkan::Headers doctest::doctest)
    add_executable(core_tests tests/core_tests.cpp)
    target_include_directories(core_tests PRIVATE "${GENERATED_DIR}")
    target_link_libraries(core_tests PRIVATE eyecal_core doctest::doctest)
    add_test(NAME m1_surface_policy COMMAND surface_tests)
    add_test(NAME m1_no_srgb_formats
        COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/no_srgb_formats.cmake)
    add_test(NAME m2_guide_and_quantize COMMAND core_tests)
    add_executable(interaction_tests tests/interaction_tests.cpp)
    target_include_directories(interaction_tests PRIVATE "${GENERATED_DIR}")
    target_link_libraries(interaction_tests PRIVATE eyecal_core doctest::doctest)
    add_test(NAME mouse_and_keyboard_interaction COMMAND interaction_tests)
endif()
if(NOT EYECAL_BUILD_APP)
    return()
endif()

find_program(GLSLC NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    URL https://codeload.github.com/libsdl-org/SDL/tar.gz/refs/tags/release-3.2.26
    URL_HASH SHA256=73eeeec0d3235ef75f310bd2bb758622157abfa05da65771656579a7dfeeba05)
FetchContent_Declare(volk
    URL https://codeload.github.com/zeux/volk/tar.gz/refs/tags/vulkan-sdk-1.4.321.0
    URL_HASH SHA256=021ed905eea6f3e2cc8a60a21459ee7b43f98f32052ca30f27a3be390b7a4862)
FetchContent_MakeAvailable(SDL3 volk)
target_link_libraries(volk PUBLIC Vulkan::Headers)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/assets/font/OFL.txt" font_license)
file(READ "${sdl3_SOURCE_DIR}/LICENSE.txt" sdl_license)
file(READ "${volk_SOURCE_DIR}/LICENSE.md" volk_license)
file(READ "${nlohmann_json_SOURCE_DIR}/LICENSE.MIT" json_license)
configure_file(cmake/licenses.hpp.in "${GENERATED_DIR}/licenses.hpp" @ONLY)
foreach(shader_name IN ITEMS quad.vert overlay.frag ramp.frag)
    string(REPLACE "." "_" shader_symbol "${shader_name}")
    set(shader "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader_name}")
    set(blob "${GENERATED_DIR}/${shader_symbol}.spv")
    set(header "${GENERATED_DIR}/${shader_symbol}.hpp")
    add_custom_command(OUTPUT "${header}"
        BYPRODUCTS "${blob}"
        COMMAND "${GLSLC}" --target-env=vulkan1.3 -O
                "-I${CMAKE_CURRENT_SOURCE_DIR}/shaders/include"
                "-I${CMAKE_CURRENT_SOURCE_DIR}/assets/blue-noise" "${shader}" -o "${blob}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${blob}" "-DOUTPUT=${header}"
                "-DNAME=${shader_symbol}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_spirv.cmake"
        DEPENDS "${shader}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_spirv.cmake"
                "${CMAKE_CURRENT_SOURCE_DIR}/shaders/include/quantize.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/blue-noise/blue_noise_ranks.inc"
        VERBATIM)
    list(APPEND shader_headers "${header}")
endforeach()

add_executable(EyeCal WIN32 src/app/main.cpp src/app/guide_input.cpp src/vk/renderer.cpp src/vk/quad.cpp
    src/ui/overlay.cpp src/ui/guide_view.cpp src/patterns/ramp.cpp ${shader_headers})
target_include_directories(EyeCal PRIVATE src "${GENERATED_DIR}")
target_link_libraries(EyeCal PRIVATE eyecal_core SDL3::SDL3-static volk::volk)
if(WIN32)
    target_sources(EyeCal PRIVATE src/platform/windows.cpp)
    target_link_libraries(EyeCal PRIVATE dxgi user32)
    if(MINGW)
        target_link_options(EyeCal PRIVATE -static)
    endif()
else()
    target_sources(EyeCal PRIVATE src/platform/portable.cpp)
endif()
if(MSVC)
    target_compile_options(EyeCal PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(EyeCal PRIVATE -Wall -Wextra -Wpedantic)
endif()
