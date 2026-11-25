#include "ui/guide_view.hpp"
#include "patterns/ramp.hpp"

#include <algorithm>
#include <utility>

namespace calib::ui {
namespace {

VkRect2D vkRect(Rect rect)
{
    return {{rect.x, rect.y}, {static_cast<std::uint32_t>(rect.width),
                              static_cast<std::uint32_t>(rect.height)}};
}

struct Button {
    Action action;
    std::string label;
    bool enabled = true;
    bool selected = false;
};

struct ButtonRow {
    std::vector<HitTarget> targets;
    int height = 0;
};

ButtonRow layoutButtons(std::span<const Button> buttons, int width, int minHeight,
                        int scale, int gap)
{
    ButtonRow layout;
    if (buttons.empty() || panelColumns(width, scale) < 1) {
        return layout;
    }
    constexpr int comfortableGlyphs = 10;
    const int comfortableWidth = (24 + 12 * comfortableGlyphs) * scale;
    const auto perRow = static_cast<std::size_t>(std::max(1, (width + gap) / (comfortableWidth + gap)));
    for (std::size_t start = 0; start < buttons.size(); start += perRow) {
        const auto count = static_cast<int>(std::min(perRow, buttons.size() - start));
        const int cellWidth = (width - (count - 1) * gap) / count;
        int rowHeight = minHeight;
        for (int j = 0; j < count; ++j) {
            const std::array<std::string_view, 1> label{buttons[start + j].label};
            const auto wrapped = wrapText(label, panelColumns(cellWidth, scale));
            rowHeight = std::max(rowHeight, (25 * static_cast<int>(wrapped.size()) + 7) * scale);
        }
        for (int j = 0; j < count; ++j) {
            const auto& button = buttons[start + j];
            layout.targets.push_back({{j * (cellWidth + gap), layout.height, cellWidth, rowHeight},
                                      button.action, button.enabled});
        }
        layout.height += rowHeight + gap;
    }
    layout.height -= gap;
    return layout;
}

void place(ButtonRow& row, int x, int y, std::vector<HitTarget>& all)
{
    for (auto& target : row.targets) {
        target.bounds.x += x;
        target.bounds.y += y;
        all.push_back(target);
    }
}

PanelStyle buttonStyle(bool enabled, bool selected, bool hovered, bool pressed, bool focused)
{
    PanelStyle style;
    style.centered = true;
    style.fill = theme::button;
    if (!enabled) {
        style.fill = theme::panel;
        style.text = theme::muted;
    } else if (pressed) {
        style.fill = theme::shade(theme::selected, -0.035f);
    } else if (focused) {
        style.fill = theme::focus;
        style.outline = 0.26f;
    } else if (hovered) {
        style.fill = theme::shade(theme::button, 0.04f);
    } else if (selected) {
        style.fill = theme::selected;
        style.outline = 0.18f;
    }
    return style;
}

void drawButtons(VkCommandBuffer command, vk::QuadPipeline overlay, VkExtent2D extent,
                 std::span<const Button> buttons, const ButtonRow& row, int scale,
                 const Pointer& pointer, std::optional<Action> hovered, float motion)
{
    for (std::size_t i = 0; i < row.targets.size(); ++i) {
        const auto& button = buttons[i];
        const auto rect = row.targets[i].bounds;
        const bool isHovered = hovered == button.action;
        const bool focused = pointer.keyboardFocus() == button.action;
        auto style = buttonStyle(button.enabled, button.selected, isHovered,
                                       isHovered && pointer.pressed() == button.action, focused);
        style.fill = theme::shade(style.fill, motion * (isHovered || focused ? -0.045f : -0.012f));
        switch (button.action.command) {
        case Command::back: style.icon = Icon::back; break;
        case Command::next: style.icon = Icon::next; break;
        case Command::menu: style.icon = Icon::menu; break;
        default: break;
        }
        const std::array<std::string_view, 1> label{button.label};
        const auto wrapped = wrapText(label, panelColumns(rect.width, scale));
        drawTextPanel(command, overlay, extent, vkRect(rect), wrapped, scale, style);
    }
}

} // namespace

std::vector<HitTarget> drawGuide(VkCommandBuffer command, vk::QuadPipeline overlay,
                                 vk::QuadPipeline ramp, VkExtent2D extent,
                                 patterns::OutputDepth depth, const guide::Session& session,
                                 std::span<const std::string_view> status, const Pointer& pointer,
                                 bool pointerVisible, float motion)
{
    const auto& screen = session.screen();
    const auto& controls = session.controls();
    const int scale = session.textScale();
    const auto width = static_cast<int>(extent.width);
    const auto height = static_cast<int>(extent.height);
    const int margin = std::min({theme::spacing * scale, width / 4, height / 4});
    const int gap = std::max(1, margin / 2);
    const int innerWidth = width - 2 * margin;
    const int columns = panelColumns(innerWidth, scale);
    const int minButtonHeight = (height * 6 + 99) / 100;
    const bool menu = session.menuOpen();
    const bool showRamp = screen.pattern != guide::Pattern::none && !menu;

    std::string dither;
    std::vector<Button> settings;
    const std::string step = std::to_string(session.step()) + controls.stepSeparator +
                             std::to_string(session.stepCount());
    const int progressHeight = menu ? 0 : 3 * scale + gap;
    std::vector<std::string_view> top;
    std::vector<std::string_view> question;
    std::vector<std::string_view> shortcuts;
    std::vector<Button> choices;
    std::vector<Button> navigation;
    if (menu) {
        top = {controls.menuTitle, controls.menuHelp};
        for (std::size_t i = 0; i < controls.menuChoices.size(); ++i) {
            auto label = controls.menuChoices[i];
            if (i == 2) {
                label += ": " + controls.toggleStates[session.reduceMotion() ? 1 : 0];
            }
            choices.push_back({{Command::menuChoice, i}, std::move(label),
                               true, session.menuChoice() == i});
        }
    } else {
        top.push_back(screen.headline);
        top.push_back(step);
        for (const auto& item : screen.instructions) {
            top.push_back(item);
        }
        if (!screen.displayHints.empty()) {
            top.push_back(screen.displayHints[session.setting(guide::Setting::display)]);
        }
        for (std::size_t i = 0; i < screen.settings.size(); ++i) {
            const auto& setting = screen.settings[i];
            const auto selected = session.setting(setting.kind);
            settings.push_back({{Command::setting, i}, setting.label + ": " + setting.options[selected]});
            if (!setting.descriptions.empty()) {
                top.push_back(setting.descriptions[selected]);
            }
        }
        top.push_back(screen.shouldSee);
        if (screen.pattern == guide::Pattern::ramp) {
            dither = screen.patternLabel + "   " + controls.ditherLabel + ": " +
                     controls.ditherModes[static_cast<std::size_t>(session.dither())];
            top.push_back(dither);
        } else if (showRamp) {
            top.push_back(screen.patternLabel);
        }
        top.push_back(screen.badExample);

        question = {screen.question};
        shortcuts = {controls.answer, controls.navigation, screen.pattern == guide::Pattern::ramp ? controls.preview : controls.intro};
        for (std::size_t i = 0; i < screen.answers.size(); ++i) {
            const bool selected = session.answer() == i;
            const auto marker = selected ? "[x] " : "[ ] ";
            choices.push_back({{Command::answer, i},
                std::to_string(i + 1) + " " + marker + screen.answers[i], true, selected});
        }
        navigation = {
            {{Command::back}, controls.navigationButtons[0], session.canGoBack()},
            {{Command::next}, controls.navigationButtons[1], session.canGoNext()},
            {{Command::menu}, controls.navigationButtons[2]}
        };
    }

    auto settingRow = layoutButtons(settings, innerWidth, minButtonHeight, scale, gap);
    const int settingsHeight = settingRow.height ? settingRow.height + gap : 0;
    auto choiceRow = layoutButtons(choices, innerWidth, minButtonHeight, scale, gap);
    auto navRow = layoutButtons(navigation, innerWidth, minButtonHeight, scale, gap);
    const auto questionLines = wrapText(question, columns);
    auto shortcutLines = wrapText(shortcuts, columns);
    const auto topLines = wrapText(top, columns);
    const auto statusLines = wrapText(status, columns);
    const auto questionHeight = questionLines.empty() ? 0 : panelHeight(questionLines.size(), scale) + gap;
    const auto topHeight = panelHeight(topLines.size(), scale);
    const auto statusHeight = statusLines.empty() ? 0 : panelHeight(statusLines.size(), scale) + margin;
    const auto middleTop = margin + topHeight + statusHeight + margin + progressHeight;
    const auto actionHeight = settingsHeight + questionHeight + choiceRow.height + (navRow.height ? gap + navRow.height : 0);
    const auto helpHeight = shortcutLines.empty() ? 0 : gap + panelHeight(shortcutLines.size(), scale);
    int bottomTop = height - margin - actionHeight - helpHeight;
    // Keep controls and the pattern reachable at small resolutions. The keyboard help remains
    // in the README when its full text cannot fit the reserved UI bands.
    if (bottomTop - margin - middleTop < (showRamp ? 24 : 0)) {
        bottomTop += helpHeight;
        shortcutLines.clear();
    }
    const int middleHeight = bottomTop - margin - middleTop;
    std::vector<HitTarget> targets;
    if (columns < 1 || actionHeight > height - 2 * margin) {
        const std::array<std::string_view, 1> message{controls.textOverflow};
        drawTextPanel(command, overlay, extent,
                      vkRect({margin, margin, innerWidth, height - 2 * margin}),
                      wrapText(message, columns), scale);
        return targets;
    }

    if (middleHeight < (showRamp ? 24 : 0)) {
        std::vector<std::string_view> message{controls.textOverflow};
        message.insert(message.end(), status.begin(), status.end());
        drawTextPanel(command, overlay, extent,
                      vkRect({margin, margin, innerWidth, std::max(0, bottomTop - 2 * margin)}),
                      wrapText(message, columns), scale);
    } else {
        int y = margin;
        if (!statusLines.empty()) {
            drawTextPanel(command, overlay, extent,
                          vkRect({margin, y, innerWidth, statusHeight - margin}), statusLines, scale);
            y += statusHeight;
        }
        PanelStyle heading;
        heading.heading = true;
        heading.fill = theme::shade(heading.fill, -0.025f * motion);
        drawTextPanel(command, overlay, extent, vkRect({margin, y, innerWidth, topHeight}),
                      topLines, scale, heading);
        if (!menu) {
            const int trackY = y + topHeight + gap;
            drawShape(command, overlay, extent, vkRect({margin, trackY, innerWidth, 3 * scale}),
                      theme::button, theme::radius);
            const int filled = static_cast<int>(innerWidth * session.step() / session.stepCount());
            drawShape(command, overlay, extent, vkRect({margin, trackY, filled, 3 * scale}),
                      theme::accent, theme::radius);
        }
        if (showRamp) {
            const int thumbnailWidth = innerWidth / 4;
            const int rampWidth = innerWidth - thumbnailWidth - margin;
            const auto gamma = static_cast<std::uint32_t>(session.setting(guide::Setting::gamma));
            patterns::drawRamp(command, ramp, extent,
                               vkRect({margin, middleTop, rampWidth, middleHeight}),
                               depth, session.dither(), false, screen.pattern, gamma);
            if (screen.pattern == guide::Pattern::sharpness) {
                for (const auto marker : {
                    Rect{width / 2 - 12, 0, 24, 1}, Rect{width / 2 - 12, height - 1, 24, 1},
                    Rect{0, height / 2 - 12, 1, 24}, Rect{width - 1, height / 2 - 12, 1, 24}
                }) {
                    patterns::drawRamp(command, ramp, extent, vkRect(marker), depth,
                                       patterns::Dither::off, false, screen.pattern);
                }
            }
            const int thumbnailX = margin * 2 + rampWidth;
            const int border = std::min(2 * scale, gap);
            for (const auto edge : {
                Rect{thumbnailX - border, middleTop - border, thumbnailWidth + 2 * border, border},
                Rect{thumbnailX - border, middleTop + middleHeight, thumbnailWidth + 2 * border, border},
                Rect{thumbnailX - border, middleTop, border, middleHeight},
                Rect{thumbnailX + thumbnailWidth, middleTop, border, middleHeight}
            }) {
                drawShape(command, overlay, extent, vkRect(edge), theme::button, 0.0f);
            }
            patterns::drawRamp(command, ramp, extent,
                               vkRect({thumbnailX, middleTop, thumbnailWidth, middleHeight}),
                               depth, patterns::Dither::off, true, screen.pattern, gamma);
        }
    }

    place(settingRow, margin, bottomTop, targets);
    if (!questionLines.empty()) {
        drawTextPanel(command, overlay, extent,
                      vkRect({margin, bottomTop + settingsHeight, innerWidth, questionHeight - gap}),
                      questionLines, scale);
    }
    const int choiceTop = bottomTop + settingsHeight + questionHeight;
    place(choiceRow, margin, choiceTop, targets);
    place(navRow, margin, choiceTop + choiceRow.height + gap, targets);
    const auto hovered = pointerVisible || pointer.pressed() ? pointer.hovered(targets) : std::nullopt;
    drawButtons(command, overlay, extent, settings, settingRow, scale, pointer, hovered, motion);
    drawButtons(command, overlay, extent, choices, choiceRow, scale, pointer, hovered, motion);
    drawButtons(command, overlay, extent, navigation, navRow, scale, pointer, hovered, motion);
    if (!shortcutLines.empty()) {
        drawTextPanel(command, overlay, extent,
                      vkRect({margin, bottomTop + actionHeight + gap, innerWidth, helpHeight - gap}),
                      shortcutLines, scale);
    }
    return targets;
}

} // namespace calib::ui
