#include "ui/overlay.hpp"
#include "ui/sdf_font.hpp"

#include "overlay_frag.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace calib::ui {
namespace {

struct Push {
    std::array<int, 4> rect{};
    std::array<int, 2> viewport{};
    float cellScale = 1;
    int solid = 0;
    std::array<std::uint32_t, 16> glyph{};
    theme::Colour ink{};
    std::array<float, 4> shape{};
};
static_assert(offsetof(Push, glyph) == 32);
static_assert(offsetof(Push, ink) == 96);
static_assert(sizeof(Push) == 128);

std::size_t utf8Length(unsigned char lead)
{
    if (lead < 0xc0u) {
        return 1;
    }
    if (lead < 0xe0u) {
        return 2;
    }
    return lead < 0xf0u ? 3 : 4;
}

std::size_t glyphCount(std::string_view text)
{
    std::size_t count = 0;
    for (unsigned char ch : text) {
        if ((ch & 0xc0u) != 0x80u) {
            ++count;
        }
    }
    return count;
}

} // namespace

vk::QuadPipeline createOverlay(VkDevice device, VkFormat format)
{
    return vk::createQuadPipeline(device, format, overlay_frag, sizeof(Push), true);
}

std::vector<std::string_view> wrapText(std::span<const std::string_view> paragraphs, int columns)
{
    std::vector<std::string_view> lines;
    if (columns <= 0) {
        return lines;
    }
    for (auto remaining : paragraphs) {
        while (glyphCount(remaining) > static_cast<std::size_t>(columns)) {
            std::size_t bytes = 0;
            for (int column = 0; column < columns; ++column) {
                bytes += utf8Length(static_cast<unsigned char>(remaining[bytes]));
            }
            const auto space = remaining.rfind(' ', bytes);
            const auto split = space == 0 || space == std::string_view::npos ? bytes : space;
            lines.push_back(remaining.substr(0, split));
            remaining.remove_prefix(split);
            const auto start = remaining.find_first_not_of(' ');
            if (start == std::string_view::npos) {
                remaining = {};
            } else {
                remaining.remove_prefix(start);
            }
        }
        lines.push_back(remaining);
    }
    return lines;
}

int panelHeight(std::size_t lines, int scale)
{
    return (2 * theme::spacing + theme::baseline * static_cast<int>(lines)) * scale;
}

int panelColumns(int width, int scale)
{
    return (width / scale - 2 * theme::spacing) / glyphWidth;
}

void drawShape(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
               VkRect2D rect, theme::Colour ink, float radius, float outline, Icon icon)
{
    if (rect.extent.width == 0 || rect.extent.height == 0) {
        return;
    }
    vk::bindQuadPipeline(command, overlay, extent, rect);
    Push push{};
    push.viewport = {static_cast<int>(extent.width), static_cast<int>(extent.height)};
    push.rect = {rect.offset.x, rect.offset.y, static_cast<int>(rect.extent.width),
                 static_cast<int>(rect.extent.height)};
    push.ink = ink;
    push.solid = static_cast<int>(icon) + 1;
    push.shape = {radius, outline, 0.0f, 0.0f};
    vkCmdPushConstants(command, overlay.layout, vk::quadStages, 0, sizeof(push), &push);
    vkCmdDraw(command, 6, 1, 0, 0);
}

void drawTextPanel(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
                   VkRect2D panel, std::span<const std::string_view> lines, int scale,
                   PanelStyle style)
{
    if (lines.empty() || panel.extent.width == 0 || panel.extent.height == 0) {
        return;
    }
    vk::bindQuadPipeline(command, overlay, extent, panel);
    Push push{};
    push.viewport = {static_cast<int>(extent.width), static_cast<int>(extent.height)};
    push.cellScale = scale;
    auto draw = [&] {
        vkCmdPushConstants(command, overlay.layout, vk::quadStages, 0, sizeof(push), &push);
        vkCmdDraw(command, 6, 1, 0, 0);
    };

    drawShape(command, overlay, extent, panel, style.fill, theme::radius * scale, style.outline);
    if (style.icon != Icon::none && panel.extent.width >= static_cast<unsigned>(120 * scale)) {
        const int iconSize = 16 * scale;
        drawShape(command, overlay, extent,
                  {{panel.offset.x + theme::spacing * scale,
                    panel.offset.y + (static_cast<int>(panel.extent.height) - iconSize) / 2},
                   {static_cast<unsigned>(iconSize), static_cast<unsigned>(iconSize)}},
                  style.text, 0.0f, 0.0f, style.icon);
    }
    vk::bindQuadPipeline(command, overlay, extent, panel);
    push.solid = 0;
    push.ink = style.text;
    const auto contentHeight = (theme::baseline * static_cast<int>(lines.size()) - 5) * scale;
    int y = panel.offset.y + (style.centered
        ? (static_cast<int>(panel.extent.height) - contentHeight) / 2 : 12 * scale);
    bool first = true;
    for (auto line : lines) {
        const bool isHeading = first && style.heading &&
            glyphCount(line) * glyphWidth * theme::titleScale * scale <=
                static_cast<int>(panel.extent.width) - 2 * theme::spacing * scale;
        first = false;
        const float glyphScale = static_cast<float>(scale) * (isHeading ? theme::titleScale : 1.0f);
        const int glyphPixels = static_cast<int>(glyphWidth * glyphScale);
        const int glyphRows = static_cast<int>(glyphHeight * glyphScale);
        push.cellScale = glyphScale;
        const auto lineWidth = glyphPixels * static_cast<int>(glyphCount(line));
        int x = panel.offset.x + (style.centered
            ? (static_cast<int>(panel.extent.width) - lineWidth) / 2 : 12 * scale);
        for (unsigned char c : line) {
            if ((c & 0xc0u) == 0x80u) {
                continue;
            }
            if (c != ' ') {
                const auto glyph = c >= 32 && c <= 126 ? c : '?';
                push.glyph = glyphs[glyph - 32];
                push.rect = {x, y, glyphPixels, glyphRows};
                draw();
            }
            x += glyphPixels;
        }
        y += theme::baseline * scale;
    }
}

void drawOverlay(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
                 std::span<const std::string_view> lines)
{
    if (lines.empty()) {
        return;
    }
    std::size_t longest = 0;
    for (auto line : lines) {
        longest = std::max(longest, glyphCount(line));
    }
    const auto width = static_cast<int>(extent.width);
    const auto height = static_cast<int>(extent.height);
    const int baseWidth = 24 + glyphWidth * static_cast<int>(longest);
    const int scale = std::max(1, std::min({width / 960, (width - 24) / baseWidth,
                                          (height - 24) / panelHeight(lines.size(), 1)}));
    const int margin = std::min({12 * scale, width / 4, height / 4});
    const auto panelWidth = static_cast<std::uint32_t>(std::min(baseWidth * scale, width - 2 * margin));
    const auto panelRows = wrapText(lines, panelColumns(static_cast<int>(panelWidth), scale));
    const auto panelRowsHeight = std::min(panelHeight(panelRows.size(), scale), height - 2 * margin);
    drawTextPanel(command, overlay, extent, {{margin, margin},
        {panelWidth, static_cast<std::uint32_t>(panelRowsHeight)}}, panelRows, scale);
}

} // namespace calib::ui
