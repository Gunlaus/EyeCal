#pragma once

#include <volk.h>

struct SDL_Window;

namespace calib::platform {

enum class OutputColor { unknown, sdr, hdr };

OutputColor outputColor(SDL_Window* window);
const char* exclusiveDeviceExtension();

struct SwapchainCreation {
    VkResult result;
    bool exclusive;
};

SwapchainCreation createSwapchain(SDL_Window* window, VkPhysicalDevice physical,
                                  VkDevice device, VkSwapchainCreateInfoKHR info,
                                  bool exclusiveRequested, bool exclusiveEnabled,
                                  VkSwapchainKHR& swapchain);
void releaseExclusive(VkDevice device, VkSwapchainKHR swapchain);

} // namespace calib::platform
