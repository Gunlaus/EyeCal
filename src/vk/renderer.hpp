#pragma once

#include "platform/platform.hpp"
#include "ui/overlay.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace calib::vk {

class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void initialize(SDL_Window* window);
    void invalidate() { rebuild_ = true; }
    void dropSwapchain();

    // False means the window or swapchain is still settling. Try again on the next event tick.
    bool draw(bool diagnostics, bool modeRequested, bool focused,
              platform::OutputColor outputColor, std::string_view notice,
              std::span<const std::string_view> licensePage);

private:
    bool buildSwapchain(bool exclusiveRequested);
    void record(std::uint32_t index, bool diagnostics, bool modeRequested,
                platform::OutputColor outputColor, std::string_view notice,
                std::span<const std::string_view> licensePage);

    SDL_Window* window_ = nullptr;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool pool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_ = VK_NULL_HANDLE;
    VkFence frameFence_ = VK_NULL_HANDLE;
    VkSemaphore acquired_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    ui::OverlayPipelines overlay_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
    std::vector<VkSemaphore> presented_;
    bool exclusiveEnabled_ = false;
    bool exclusiveAcquired_ = false;
    bool rebuild_ = true;
};

} // namespace calib::vk
