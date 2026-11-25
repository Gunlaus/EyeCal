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
    throw std::runtime_error("Unknown guide pattern: " + std::string(name));
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
             {required(c, "resume"), required(c, "exit")}},
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
                patternFrom(required(entry, "pattern")), entry.at("pattern_label").get<std::string>()
            };
            if (!ids.insert(screen.id).second || screen.instructions.empty() ||
                !allNonempty(screen.instructions) || screen.answers.size() < 2 ||
                !allNonempty(screen.answers) ||
                (screen.pattern != Pattern::none && screen.patternLabel.empty())) {
                throw std::runtime_error("Invalid screen data for " + screen.id);
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

patterns::Dither Session::dither() const
{
    return screen().pattern == Pattern::ramp ? previewDither_ : patterns::Dither::off;
}

void Session::selectAnswer(std::size_t answer)
{
    if (!menuOpen_ && answer < screen().answers.size()) {
        answers_[index_] = answer;
    }
}

bool Session::activateMenuChoice(std::size_t choice)
{
    if (!menuOpen_ || choice >= controls().menuChoices.size()) {
        return false;
    }
    menuChoice_ = choice;
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
            menuChoice_ = menuChoice_ == 0 ? 1 : 0;
        } else if (input == Input::confirm) {
            return activateMenuChoice(menuChoice_);
        }
        return false;
    }
    switch (input) {
    case Input::previous:
        if (canGoBack()) {
            --index_;
        }
        break;
    case Input::next:
    case Input::confirm:
        if (canGoNext()) {
            ++index_;
        }
        break;
    case Input::up:
    case Input::down: {
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
        if (screen().pattern == Pattern::ramp) {
            const auto next = (static_cast<std::size_t>(previewDither_) + 1) % controls().ditherModes.size();
            previewDither_ = static_cast<patterns::Dither>(next);
        }
        break;
    default:
        break;
    }
    return false;
}

} // namespace calib::guide
