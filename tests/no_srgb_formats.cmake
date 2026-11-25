file(GLOB_RECURSE project_sources "${SOURCE_DIR}/src/*.cpp" "${SOURCE_DIR}/src/*.hpp"
                                 "${SOURCE_DIR}/shaders/*.glsl" "${SOURCE_DIR}/shaders/*.vert"
                                 "${SOURCE_DIR}/shaders/*.frag")
foreach(path IN LISTS project_sources)
    file(READ "${path}" content)
    if(content MATCHES "VK_FORMAT_[A-Z0-9_]*_SRGB([_A-Z0-9]*|$)")
        message(FATAL_ERROR "Disallowed sRGB image format in ${path}")
    endif()
endforeach()
