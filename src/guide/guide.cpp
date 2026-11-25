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
    if (name == "levels") return Pattern::levels;
    if (name == "black") return Pattern::black;
    if (name == "white") return Pattern::white;
    if (name == "gamma") return Pattern::gamma;
    if (name == "sharpness") return Pattern::sharpness;
    if (name == "banding_split") return Pattern::bandingSplit;
    if (name == "banding_depth") return Pattern::bandingDepth;
    if (name == "banding_dark") return Pattern::bandingDark;
    if (name == "grey_balance") return Pattern::greyBalance;
    if (name == "colour_steps") return Pattern::colourSteps;
    if (name == "channel_clip") return Pattern::channelClip;
    if (name == "uniformity") return Pattern::uniformity;
    if (name == "hdr_peak") return Pattern::hdrPeak;
    if (name == "hdr_black") return Pattern::hdrBlack;
    if (name == "hdr_white") return Pattern::hdrWhite;
    if (name == "hdr_windows") return Pattern::hdrWindows;
    throw std::runtime_error("Unknown guide pattern: " + std::string(name));
}

Setting settingFrom(std::string_view id)
{
    if (id == "field") return Setting::field;
    if (id == "hdr_window") return Setting::hdrWindow;
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
            {{required(c, "back"), required(c, "next"), required(c, "menu")}, required(c, "preview"),
             required(c, "intro"), required(c, "dither_label"),
             c.at("dither_modes").get<std::array<std::string, 4>>(),
             required(c, "text_overflow"), required(c, "menu_title"), required(c, "menu_help"),
             {required(c, "reduce_motion"), required(c, "exit")},
             {required(c, "off"), required(c, "on")}, required(c, "step_separator"),
             c.at("probe_results").get<std::array<std::string, 6>>(),
             c.at("depth_buttons").get<std::array<std::string, 2>>(),
             c.at("motion_buttons").get<std::array<std::string, 2>>(),
             c.at("baseline_buttons").get<std::array<std::string, 2>>(),
             c.at("display_messages").get<std::array<std::string, 5>>(),
             c.at("start_buttons").get<std::array<std::string, 3>>() },
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
                required(entry, "should_see"), entry.at("bad_example").get<std::string>(),
                entry.value("question", std::string{}), entry.value("answers", std::vector<std::string>{}),
                patternFrom(required(entry, "pattern")), entry.at("pattern_label").get<std::string>(), {}, {}, required(entry, "name")
            };
            if (!ids.insert(screen.id).second || screen.instructions.empty() ||
                !allNonempty(screen.instructions) || (!screen.answers.empty() && screen.question.empty()) ||
                !allNonempty(screen.answers) ||
                (screen.pattern != Pattern::none && screen.patternLabel.empty())) {
                throw std::runtime_error("Invalid screen data for " + screen.id);
            }
            if (entry.contains("settings")) {
                for (const auto& value : entry.at("settings")) {
                    ChoiceSetting setting{settingFrom(required(value, "id")), required(value, "label"),
                                          value.at("options").get<std::vector<std::string>>(), {}};
                    if (value.contains("descriptions")) {
                        setting.descriptions = value.at("descriptions").get<std::vector<std::string>>();
                    }
                    if (setting.options.empty() || !allNonempty(setting.options) ||
                        (!setting.descriptions.empty() && setting.descriptions.size() != setting.options.size())) {
                        throw std::runtime_error("Invalid setting options for " + screen.id);
                    }
                    screen.settings.push_back(std::move(setting));
                }
            }
            if (entry.contains("result_hints")) {
                screen.resultHints = entry.at("result_hints").get<decltype(screen.resultHints)>();
                if (screen.resultHints.size() != screen.answers.size() || !allNonempty(screen.resultHints)) {
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
    const auto& hints = screen().resultHints;
    return answer() && !hints.empty() ? std::string_view{hints[*answer()]} : std::string_view{};
}

patterns::Dither Session::dither() const
{
    const auto pattern = screen().pattern;
    if (pattern == Pattern::ramp) {
        return baselineDither_ ? patterns::Dither::blueNoise : patterns::Dither::off;
    }
    return pattern == Pattern::bandingSplit ? previewDither_ : patterns::Dither::off;
}

std::size_t Session::setting(Setting kind) const
{
    return settings_[static_cast<std::size_t>(kind)];
}

void Session::cycleSetting(std::size_t localIndex, int direction)
{
    if (menuOpen_ || localIndex >= screen().settings.size()) {
        return;
    }
    const auto& options = screen().settings[localIndex];
    const std::size_t count = options.options.size();
    auto& current = settings_[static_cast<std::size_t>(options.kind)];
    current = (current + (direction < 0 ? count - 1 : 1)) % count;
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

void Session::startSuite(std::size_t suite)
{
    constexpr std::array starts{Pattern::levels, Pattern::greyBalance, Pattern::hdrPeak};
    if (suite >= starts.size()) return;
    const auto found = std::ranges::find(content_.screens, starts[suite], &Screen::pattern);
    if (found != content_.screens.end()) {
        jump(static_cast<std::size_t>(found - content_.screens.begin()));
    }
}

std::string Session::menuLabel(std::size_t choice) const
{
    const auto count = content_.screens.size();
    if (choice < count) return content_.screens[choice].name;
    if (choice == count) return controls().menuChoices[0] + ": " +
        controls().toggleStates[reduceMotion_ ? 1 : 0];
    return controls().menuChoices[1];
}

void Session::jump(std::size_t target)
{
    if (target == index_) return;
    index_ = target;
    if (screen().pattern == Pattern::bandingDepth) {
        tenBit_ = false;
        resetProbe();
    }
}

bool Session::activateMenuChoice(std::size_t choice)
{
    if (!menuOpen_ || choice >= menuCount()) return false;
    menuChoice_ = choice;
    const auto count = content_.screens.size();
    if (choice == count) {
        reduceMotion_ = !reduceMotion_;
        return false;
    }
    menuOpen_ = false;
    if (choice < count) {
        jump(choice);
        return false;
    }
    return true;
}

bool Session::handle(Input input)
{
    if (input == Input::textSize) {
        textScale_ = textScale_ % 3 + 1;
        return false;
    }
    if (input == Input::menu) {
        menuOpen_ = !menuOpen_;
        menuChoice_ = index_;
        return false;
    }
    if (menuOpen_) {
        if (input == Input::up || input == Input::down) {
            const auto count = menuCount();
            menuChoice_ = (menuChoice_ + (input == Input::up ? count - 1 : 1)) % count;
        } else if (input == Input::confirm) {
            return activateMenuChoice(menuChoice_);
        }
        return false;
    }
    switch (input) {
    case Input::previous:
        if (canGoBack()) {
            jump(index_ - 1);
        }
        break;
    case Input::next:
    case Input::confirm:
        if (canGoNext()) {
            jump(index_ + 1);
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
        if (screen().pattern == Pattern::ramp) baselineDither_ = !baselineDither_;
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
