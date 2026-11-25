#include "ui/guide_view.hpp"
#include "patterns/ramp.hpp"

#include <algorithm>

namespace calib::ui {
namespace {

VkRect2D rectangle(int x, int y, int width, int height)
{
    return {{x, y}, {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)}};
}

std::string marked(std::string_view label, bool selected)
{
    return (selected ? "[x] " : "[ ] ") + std::string(label);
}

} // namespace

void drawGuide(VkCommandBuffer command, vk::QuadPipeline overlay, vk::QuadPipeline ramp,
               VkExtent2D extent, patterns::OutputDepth depth, const guide::Session& session,
               std::span<const std::string_view> status)
{
    const auto& screen = session.screen();
    const auto& controls = session.controls();
    const int scale = session.textScale();
    const auto width = static_cast<int>(extent.width);
    const auto height = static_cast<int>(extent.height);
    const int margin = std::min({12 * scale, width / 4, height / 4});
    const int innerWidth = width - 2 * margin;
    const int columns = panelColumns(innerWidth, scale);
    const bool showRamp = screen.pattern == guide::Pattern::ramp && !session.menuOpen();

    std::string options;
    std::string dither;
    std::array<std::string, 2> menuChoices;
    std::vector<std::string_view> top;
    std::vector<std::string_view> bottom;
    if (session.menuOpen()) {
        top.push_back(controls.menuTitle);
        for (std::size_t i = 0; i < menuChoices.size(); ++i) {
            menuChoices[i] = marked(controls.menuChoices[i], i == session.menuChoice());
            top.push_back(menuChoices[i]);
        }
        bottom.push_back(controls.menuHelp);
    } else {
        top.push_back(screen.headline);
        for (const auto& item : screen.instructions) {
            top.push_back(item);
        }
        top.push_back(screen.shouldSee);
        if (showRamp) {
            dither = screen.patternLabel + "   " + controls.ditherLabel + ": " +
                     controls.ditherModes[static_cast<std::size_t>(session.dither())];
            top.push_back(dither);
        }
        top.push_back(screen.badExample);

        for (std::size_t i = 0; i < screen.answers.size(); ++i) {
            if (i != 0) {
                options += "   ";
            }
            options += marked(screen.answers[i], session.answer() == i);
        }
        bottom = {screen.question, options, controls.answer, controls.navigation,
                  showRamp ? controls.preview : controls.intro};
    }

    const auto topLines = wrapText(top, columns);
    const auto bottomLines = wrapText(bottom, columns);
    const auto statusLines = wrapText(status, columns);
    const auto topHeight = panelHeight(topLines.size(), scale);
    const auto bottomHeight = panelHeight(bottomLines.size(), scale);
    const auto statusHeight = statusLines.empty() ? 0 : panelHeight(statusLines.size(), scale) + margin;
    const auto middleTop = margin + topHeight + statusHeight + margin;
    const auto bottomTop = height - margin - bottomHeight;
    const auto middleHeight = bottomTop - margin - middleTop;

    if (columns < 1 || middleHeight < (showRamp ? 24 : 0)) {
        std::vector<std::string_view> message{controls.textOverflow};
        message.insert(message.end(), status.begin(), status.end());
        const auto fallback = wrapText(message, columns);
        drawTextPanel(command, overlay, extent,
                      rectangle(margin, margin, innerWidth, height - 2 * margin), fallback, scale);
        return;
    }

    int y = margin;
    if (!statusLines.empty()) {
        drawTextPanel(command, overlay, extent,
                      rectangle(margin, y, innerWidth, statusHeight - margin), statusLines, scale);
        y += statusHeight;
    }
    drawTextPanel(command, overlay, extent, rectangle(margin, y, innerWidth, topHeight),
                  topLines, scale);
    drawTextPanel(command, overlay, extent, rectangle(margin, bottomTop, innerWidth, bottomHeight),
                  bottomLines, scale);
    if (showRamp) {
        const int thumbnailWidth = innerWidth / 4;
        const int rampWidth = innerWidth - thumbnailWidth - margin;
        patterns::drawRamp(command, ramp, extent,
                           rectangle(margin, middleTop, rampWidth, middleHeight),
                           depth, session.dither(), false);
        patterns::drawRamp(command, ramp, extent,
                           rectangle(margin * 2 + rampWidth, middleTop, thumbnailWidth, middleHeight),
                           depth, patterns::Dither::off, true);
    }
}

} // namespace calib::ui
