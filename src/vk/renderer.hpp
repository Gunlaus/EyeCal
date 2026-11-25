#pragma once

#include "guide/guide.hpp"
#include "platform/platform.hpp"
#include "ui/overlay.hpp"
#include "ui/interaction.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace calib::vk {

struct Frame {
    const guide::Session& guide;
    const ui::Pointer& pointer;
    bool pointerVisible;
    float chromeMotion;
    float patternPhase;
    bool diagnostics;
    bool exclusiveRequested;
    bool focused;
    platform::OutputColor outputColor;
    std::string_view notice;
    std::span<const std::string_view> licensePage;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void initialize(SDL_Window* window);
    void invalidate() { rebuild_ = true; }
    void dropSwapchain();
    bool supportsTenBit() const { return supportsTenBit_; }
    bool activeTenBit() const;
    bool exclusiveGranted() const { return exclusiveAcquired_; }

    // False means the window or swapchain is still settling. Try again on the next event tick.
    bool draw(const Frame& frame, std::vector<ui::HitTarget>& targets);

private:
    bool buildSwapchain(bool exclusiveRequested, bool requestTenBit, bool requestHdr);
    std::vector<ui::HitTarget> record(std::uint32_t index, const Frame& frame);

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
    QuadPipeline overlay_{};
    QuadPipeline ramp_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
    std::vector<VkSemaphore> presented_;
    bool exclusiveEnabled_ = false;
    bool exclusiveAcquired_ = false;
    bool rebuild_ = true;
    bool previousDepthRequest_ = false;
    bool supportsTenBit_ = false;
    bool hdrColorspaceEnabled_ = false;
    bool hdrMetadataEnabled_ = false;
    bool previousHdrRequest_ = false;
    bool activeHdr_ = false;
};

} // namespace calib::vk
