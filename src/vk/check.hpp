#pragma once

#include <vulkan/vulkan_core.h>
#include <stdexcept>
#include <string>
#include <string_view>

namespace calib::vk {

inline void check(VkResult result, std::string_view operation)
{
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(operation) + " (Vulkan result " +
                                 std::to_string(result) + ").");
    }
}

} // namespace calib::vk
