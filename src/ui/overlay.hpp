#pragma once

#include <volk.h>
#include <span>
#include <string_view>

namespace calib::ui {

struct OverlayPipelines {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

OverlayPipelines createOverlay(VkDevice device, VkFormat format);
void destroyOverlay(VkDevice device, OverlayPipelines overlay);
void drawOverlay(VkCommandBuffer command, OverlayPipelines overlay, VkExtent2D extent,
                 std::span<const std::string_view> lines);

} // namespace calib::ui
