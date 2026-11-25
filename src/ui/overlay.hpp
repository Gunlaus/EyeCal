#pragma once

#include "vk/quad.hpp"
#include "ui/theme.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace calib::ui {

vk::QuadPipeline createOverlay(VkDevice device, VkFormat format, bool hdr);
std::vector<std::string_view> wrapText(std::span<const std::string_view> paragraphs, int columns);
int panelHeight(std::size_t lines, int scale);
int panelColumns(int width, int scale);
enum class Icon { none, back, next, menu };

struct PanelStyle {
    theme::Colour fill = theme::panel;
    theme::Colour text = theme::text;
    bool centered = false;
    bool heading = false;
    float outline = 0.07f;
    Icon icon = Icon::none;
};

void drawShape(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
               VkRect2D rect, theme::Colour ink, float radius, float outline = 0.0f,
               Icon icon = Icon::none);

void drawTextPanel(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
                   VkRect2D panel, std::span<const std::string_view> lines, int scale,
                   PanelStyle style = {});
void drawOverlay(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
                 std::span<const std::string_view> lines);

} // namespace calib::ui
