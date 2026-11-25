cmake_minimum_required(VERSION 3.24)
project(CalibGuide VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(CTest)
include(FetchContent)
option(CALIBGUIDE_BUILD_APP "Build the Vulkan executable" ON)
if(BUILD_TESTING)
    add_executable(m1_tests tests/m1_tests.cpp)
    target_include_directories(m1_tests PRIVATE src)
    add_test(NAME m1_surface_policy COMMAND m1_tests)
    add_test(NAME m1_no_srgb_formats
        COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/no_srgb_formats.cmake)
endif()
if(NOT CALIBGUIDE_BUILD_APP AND NOT BUILD_TESTING)
    return()
endif()

FetchContent_Declare(vulkan_headers
    URL https://codeload.github.com/KhronosGroup/Vulkan-Headers/tar.gz/refs/tags/v1.4.321
    URL_HASH SHA256=7c1874f9186d9e7d5245e279cdccaa77a66ed8f52871b428630aeb771dc7623c)
FetchContent_MakeAvailable(vulkan_headers)
if(BUILD_TESTING)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(doctest
        URL https://codeload.github.com/doctest/doctest/tar.gz/refs/tags/v2.4.12
        URL_HASH SHA256=73381c7aa4dee704bd935609668cf41880ea7f19fa0504a200e13b74999c2d70)
    FetchContent_MakeAvailable(doctest)
    target_link_libraries(m1_tests PRIVATE Vulkan::Headers doctest::doctest)
endif()
if(NOT CALIBGUIDE_BUILD_APP)
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

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/assets/font/OFL.txt" font_license)
file(READ "${sdl3_SOURCE_DIR}/LICENSE.txt" sdl_license)
file(READ "${volk_SOURCE_DIR}/LICENSE.md" volk_license)
configure_file(cmake/licenses.hpp.in "${GENERATED_DIR}/licenses.hpp" @ONLY)
foreach(stage IN ITEMS vert frag)
    set(shader "${CMAKE_CURRENT_SOURCE_DIR}/shaders/overlay.${stage}")
    set(blob "${GENERATED_DIR}/overlay_${stage}.spv")
    set(header "${GENERATED_DIR}/overlay_${stage}.hpp")
    add_custom_command(OUTPUT "${header}"
        BYPRODUCTS "${blob}"
        COMMAND "${GLSLC}" --target-env=vulkan1.3 -O "${shader}" -o "${blob}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${blob}" "-DOUTPUT=${header}"
                "-DNAME=overlay_${stage}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_spirv.cmake"
        DEPENDS "${shader}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_spirv.cmake"
        VERBATIM)
    list(APPEND shader_headers "${header}")
endforeach()

add_executable(CalibGuide WIN32 src/app/main.cpp src/vk/renderer.cpp src/ui/overlay.cpp ${shader_headers})
target_include_directories(CalibGuide PRIVATE src "${GENERATED_DIR}")
target_link_libraries(CalibGuide PRIVATE SDL3::SDL3-static volk::volk)
if(WIN32)
    target_sources(CalibGuide PRIVATE src/platform/windows.cpp)
    target_link_libraries(CalibGuide PRIVATE dxgi user32)
    if(MINGW)
        target_link_options(CalibGuide PRIVATE -static)
    endif()
else()
    target_sources(CalibGuide PRIVATE src/platform/portable.cpp)
endif()
if(MSVC)
    target_compile_options(CalibGuide PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(CalibGuide PRIVATE -Wall -Wextra -Wpedantic)
endif()
