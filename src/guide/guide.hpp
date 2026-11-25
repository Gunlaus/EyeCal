#pragma once

#include "patterns/quantize.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace calib::guide {

enum class Pattern { none, ramp };

struct Screen {
    std::string id;
    std::string headline;
    std::vector<std::string> instructions;
    std::string shouldSee;
    std::string badExample;
    std::string question;
    std::vector<std::string> answers;
    Pattern pattern;
    std::string patternLabel;
};

struct Controls {
    std::string navigation;
    std::string answer;
    std::array<std::string, 3> navigationButtons;
    std::string preview;
    std::string intro;
    std::string ditherLabel;
    std::array<std::string, 4> ditherModes;
    std::string textOverflow;
    std::string menuTitle;
    std::string menuHelp;
    std::array<std::string, 3> menuChoices;
    std::array<std::string, 2> toggleStates;
    std::string stepSeparator;
};

struct Content {
    Controls controls;
    std::vector<Screen> screens;
};

Content loadContent(std::string_view json);

enum class Input { previous, next, up, down, confirm, menu, dither, textSize };

class Session {
public:
    explicit Session(const Content& content);

    // A true result asks the application to exit.
    bool handle(Input input);
    void selectAnswer(std::size_t answer);
    bool activateMenuChoice(std::size_t choice);
    bool canGoBack() const { return index_ > 0; }
    bool canGoNext() const { return index_ + 1 < content_.screens.size(); }
    bool keepsCursorVisible() const { return menuOpen_ || screen().id == "summary"; }
    const Screen& screen() const { return content_.screens[index_]; }
    const Controls& controls() const { return content_.controls; }
    std::optional<std::size_t> answer() const { return answers_[index_]; }
    std::size_t step() const { return index_ + 1; }
    std::size_t stepCount() const { return content_.screens.size(); }
    bool reduceMotion() const { return reduceMotion_; }
    bool menuOpen() const { return menuOpen_; }
    std::size_t menuChoice() const { return menuChoice_; }
    patterns::Dither dither() const;
    int textScale() const { return textScale_; }

private:
    const Content& content_;
    std::vector<std::optional<std::size_t>> answers_;
    std::size_t index_ = 0;
    std::size_t menuChoice_ = 0;
    bool menuOpen_ = false;
    bool reduceMotion_ = false;
    patterns::Dither previewDither_ = patterns::Dither::off;
    int textScale_ = 1;
};

} // namespace calib::guide
