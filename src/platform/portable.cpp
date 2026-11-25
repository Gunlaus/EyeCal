#include "platform/platform.hpp"

namespace calib::platform {

OutputColor outputColor(SDL_Window*)
{
    return OutputColor::unknown;
}

const char* exclusiveDeviceExtension()
{
    return nullptr;
}

SwapchainCreation createSwapchain(SDL_Window*, VkPhysicalDevice, VkDevice device,
                                  VkSwapchainCreateInfoKHR info, bool, bool,
                                  VkSwapchainKHR& swapchain)
{
    return {vkCreateSwapchainKHR(device, &info, nullptr, &swapchain), false};
}

void releaseExclusive(VkDevice, VkSwapchainKHR)
{
}

} // namespace calib::platform
