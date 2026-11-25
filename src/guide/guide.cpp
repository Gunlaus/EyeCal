#include "guide/guide.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <set>
#include <stdexcept>
#include <utility>

namespace calib::guide {
namespace {

std::string required(const nlohmann::json& obj, std::string_view key)
{
    auto value = obj.at(key).get<std::string>();
    if (value.empty()) {
        throw std::runtime_error("Empty text for " + std::string(key));
    }
    return value;
}

Pattern patternFrom(std::string_view name)
{
    if (name == "none") {
        return Pattern::none;
    }
    if (name == "ramp") {
        return Pattern::ramp;
    }
    if (name == "setup") return Pattern::setup;
    if (name == "levels") return Pattern::levels;
    if (name == "black") return Pattern::black;
    if (name == "white") return Pattern::white;
    if (name == "gamma") return Pattern::gamma;
    if (name == "sharpness") return Pattern::sharpness;
    if (name == "banding_split") return Pattern::bandingSplit;
    if (name == "banding_depth") return Pattern::bandingDepth;
    if (name == "banding_dark") return Pattern::bandingDark;
    throw std::runtime_error("Unknown guide pattern: " + std::string(name));
}

Setting settingFrom(std::string_view id)
{
    if (id == "display") return Setting::display;
    if (id == "panel") return Setting::panel;
    if (id == "gamma") return Setting::gamma;
    throw std::runtime_error("Unknown guide setting: " + std::string(id));
}

bool allNonempty(const auto& strings)
{
    return std::ranges::none_of(strings, [](const auto& value) { return value.empty(); });
}

} // namespace

Content loadContent(std::string_view text)
{
    try {
        const auto root = nlohmann::json::parse(text);
        const auto& c = root.at("controls");
        Content content{
            {required(c, "navigation"), required(c, "answer"),
             {required(c, "back"), required(c, "next"), required(c, "menu")}, required(c, "preview"),
             required(c, "intro"), required(c, "dither_label"),
             c.at("dither_modes").get<std::array<std::string, 4>>(),
             required(c, "text_overflow"), required(c, "menu_title"), required(c, "menu_help"),
             {required(c, "resume"), required(c, "exit"), required(c, "reduce_motion")},
             {required(c, "off"), required(c, "on")}, required(c, "step_separator"),
             c.at("probe_results").get<std::array<std::string, 6>>(),
             c.at("depth_buttons").get<std::array<std::string, 2>>(),
             c.at("motion_buttons").get<std::array<std::string, 2>>() },
            {}
        };
        if (c.at("dither_modes").size() != content.controls.ditherModes.size() ||
            !allNonempty(content.controls.ditherModes)) {
            throw std::runtime_error("Expected four dither labels");
        }
        const auto& screens = root.at("screens");
        if (!screens.is_array() || screens.empty()) {
            throw std::runtime_error("Expected at least one screen");
        }
        std::set<std::string> ids;
        for (const auto& entry : screens) {
            Screen screen{
                required(entry, "id"), required(entry, "headline"),
                entry.at("instructions").get<std::vector<std::string>>(),
                required(entry, "should_see"), required(entry, "bad_example"),
                required(entry, "question"), entry.at("answers").get<std::vector<std::string>>(),
                patternFrom(required(entry, "pattern")), entry.at("pattern_label").get<std::string>(), {}, {}, {}
            };
            if (!ids.insert(screen.id).second || screen.instructions.empty() ||
                !allNonempty(screen.instructions) || screen.answers.size() < 2 ||
                !allNonempty(screen.answers) ||
                (screen.pattern != Pattern::none && screen.patternLabel.empty())) {
                throw std::runtime_error("Invalid screen data for " + screen.id);
            }
            if (entry.contains("display_hints")) {
                screen.displayHints = entry.at("display_hints").get<std::vector<std::string>>();
                if (screen.displayHints.size() != 3 || !allNonempty(screen.displayHints)) {
                    throw std::runtime_error("Expected three display hints for " + screen.id);
                }
            }
            if (entry.contains("settings")) {
                for (const auto& value : entry.at("settings")) {
                    ChoiceSetting setting{settingFrom(required(value, "id")), required(value, "label"),
                                          value.at("options").get<std::vector<std::string>>(), {}};
                    if (value.contains("descriptions")) {
                        setting.descriptions = value.at("descriptions").get<std::vector<std::string>>();
                    }
                    if (setting.options.size() != 3 || !allNonempty(setting.options) ||
                        (!setting.descriptions.empty() && setting.descriptions.size() != 3)) {
                        throw std::runtime_error("Invalid setting options for " + screen.id);
                    }
                    screen.settings.push_back(std::move(setting));
                }
            }
            if (entry.contains("result_hints")) {
                screen.resultHints = entry.at("result_hints").get<decltype(screen.resultHints)>();
                if (!std::ranges::all_of(screen.resultHints, [&](const auto& hints) {
                    return hints.size() == screen.answers.size() && allNonempty(hints);
                })) {
                    throw std::runtime_error("Invalid result hints for " + screen.id);
                }
            }
            content.screens.push_back(std::move(screen));
        }
        return content;
    } catch (const std::exception& error) {
        throw std::runtime_error(std::string("Load embedded en.json: ") + error.what());
    }
}

Session::Session(const Content& content) : content_(content), answers_(content.screens.size())
{
}

namespace {
bool laterBandingPart(Pattern pattern)
{
    return pattern == Pattern::bandingSplit || pattern == Pattern::bandingDepth ||
           pattern == Pattern::bandingDark;
}
} // namespace

std::size_t Session::step() const
{
    return static_cast<std::size_t>(std::count_if(content_.screens.begin(),
        content_.screens.begin() + index_ + 1, [](const Screen& value) {
            return !laterBandingPart(value.pattern);
        }));
}

std::size_t Session::stepCount() const
{
    return static_cast<std::size_t>(std::ranges::count_if(content_.screens, [](const Screen& value) {
        return !laterBandingPart(value.pattern);
    }));
}

std::string_view Session::resultHint() const
{
    if (screen().pattern == Pattern::bandingDepth) {
        return controls().probeResults[static_cast<std::size_t>(probe_.result(answer()))];
    }
    const auto& hints = screen().resultHints[displayType_];
    return answer() && !hints.empty() ? std::string_view{hints[*answer()]} : std::string_view{};
}

patterns::Dither Session::dither() const
{
    return screen().pattern == Pattern::bandingSplit ? previewDither_ : patterns::Dither::off;
}

std::size_t Session::setting(Setting kind) const
{
    switch (kind) {
    case Setting::display: return displayType_;
    case Setting::panel: return panelType_;
    case Setting::gamma: return gamma_.value_or(displayType_ == 1 ? 2 : displayType_ == 2 ? 1 : 0);
    }
    return 0;
}

void Session::cycleSetting(std::size_t localIndex, int direction)
{
    if (menuOpen_ || localIndex >= screen().settings.size()) {
        return;
    }
    const auto& options = screen().settings[localIndex];
    const std::size_t count = options.options.size();
    const auto next = (setting(options.kind) + (direction < 0 ? count - 1 : 1)) % count;
    switch (options.kind) {
    case Setting::display: displayType_ = next; break;
    case Setting::panel: panelType_ = next; break;
    case Setting::gamma: gamma_ = next; break;
    }
}

void Session::resetProbe()
{
    probe_.reset();
    if (screen().pattern == Pattern::bandingDepth) answers_[index_].reset();
}

void Session::selectAnswer(std::size_t answer)
{
    if (!menuOpen_ && canAnswer() && answer < screen().answers.size()) {
        answers_[index_] = answer;
    }
}

bool Session::activateMenuChoice(std::size_t choice)
{
    if (!menuOpen_ || choice >= controls().menuChoices.size()) {
        return false;
    }
    menuChoice_ = choice;
    if (choice == 2) {
        reduceMotion_ = !reduceMotion_;
        return false;
    }
    menuOpen_ = false;
    return choice == 1;
}

bool Session::handle(Input input)
{
    if (input == Input::textSize) {
        textScale_ = textScale_ % 3 + 1;
        return false;
    }
    if (input == Input::menu) {
        menuOpen_ = !menuOpen_;
        menuChoice_ = 0;
        return false;
    }
    if (menuOpen_) {
        if (input == Input::up || input == Input::down) {
            const auto count = controls().menuChoices.size();
            menuChoice_ = (menuChoice_ + (input == Input::up ? count - 1 : 1)) % count;
        } else if (input == Input::confirm) {
            return activateMenuChoice(menuChoice_);
        }
        return false;
    }
    switch (input) {
    case Input::previous:
        if (canGoBack()) {
            --index_;
            if (screen().pattern == Pattern::bandingDepth) { tenBit_ = false; resetProbe(); }
        }
        break;
    case Input::next:
    case Input::confirm:
        if (canGoNext()) {
            ++index_;
            if (screen().pattern == Pattern::bandingDepth) { tenBit_ = false; resetProbe(); }
        }
        break;
    case Input::up:
    case Input::down: {
        if (!canAnswer()) break;
        const auto last = screen().answers.size() - 1;
        auto& choice = answers_[index_];
        if (!choice) {
            choice = input == Input::up ? last : 0;
        } else {
            choice = input == Input::up ? (*choice + last) % (last + 1)
                                        : (*choice + 1) % (last + 1);
        }
        break;
    }
    case Input::dither:
        if (screen().pattern == Pattern::bandingSplit) {
            const auto next = static_cast<std::size_t>(previewDither_) % 3 + 1;
            previewDither_ = static_cast<patterns::Dither>(next);
        }
        break;
    case Input::bitDepth:
        if (screen().pattern == Pattern::bandingDepth && probe_.available()) tenBit_ = !tenBit_;
        break;
    case Input::movePattern:
        if (screen().pattern == Pattern::bandingDark) moving_ = !moving_;
        break;
    default:
        break;
    }
    return false;
}

} // namespace calib::guide
