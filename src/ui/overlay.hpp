#pragma once

#include "vk/quad.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace calib::ui {

vk::QuadPipeline createOverlay(VkDevice device, VkFormat format);
std::vector<std::string_view> wrapText(std::span<const std::string_view> paragraphs, int columns);
int panelHeight(std::size_t lines, int scale);
int panelColumns(int width, int scale);
struct PanelStyle {
    std::array<float, 4> fill{0.025f, 0.025f, 0.025f, 1.0f};
    std::array<float, 4> text{0.96f, 0.96f, 0.96f, 1.0f};
    bool centered = false;
};

void drawTextPanel(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
                   VkRect2D panel, std::span<const std::string_view> lines, int scale,
                   PanelStyle style = {});
void drawOverlay(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
                 std::span<const std::string_view> lines);

} // namespace calib::ui
