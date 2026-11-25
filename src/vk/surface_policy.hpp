#pragma once

#include <vulkan/vulkan_core.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace calib::vk {

inline std::optional<VkSurfaceFormatKHR> chooseSurfaceFormat(
    std::span<const VkSurfaceFormatKHR> formats)
{
    constexpr std::array preferred{VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
    constexpr auto sdr = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    for (auto requested : preferred) {
        for (const auto& format : formats) {
            if (format.colorSpace == sdr && format.format == requested) {
                return VkSurfaceFormatKHR{requested, sdr};
            }
        }
    }
    return std::nullopt;
}

inline bool equalExtent(VkExtent2D a, VkExtent2D b)
{
    return a.width == b.width && a.height == b.height;
}

inline std::optional<VkExtent2D> chooseNativeExtent(
    const VkSurfaceCapabilitiesKHR& caps, VkExtent2D framebuffer)
{
    if (framebuffer.width == 0 || framebuffer.height == 0) {
        return std::nullopt;
    }
    if (caps.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        return equalExtent(caps.currentExtent, framebuffer) ? std::optional{framebuffer} : std::nullopt;
    }
    if (framebuffer.width < caps.minImageExtent.width || framebuffer.width > caps.maxImageExtent.width ||
        framebuffer.height < caps.minImageExtent.height || framebuffer.height > caps.maxImageExtent.height) {
        return std::nullopt;
    }
    return framebuffer;
}

inline std::uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& caps)
{
    const auto wanted = std::max(caps.minImageCount, 3u);
    return caps.maxImageCount == 0 ? wanted : std::min(wanted, caps.maxImageCount);
}

} // namespace calib::vk
