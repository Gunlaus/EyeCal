#include "app/guide_input.hpp"

#include <algorithm>

namespace calib::app {
namespace {

using guide::Input;
using ui::Action;
using ui::Command;
using Clock = ui::CursorTimer::Clock;

std::optional<Input> navigationKey(SDL_Keycode key)
{
    switch (key) {
    case SDLK_LEFT: return Input::previous;
    case SDLK_RIGHT: return Input::next;
    case SDLK_UP: return Input::up;
    case SDLK_DOWN: return Input::down;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: return Input::confirm;
    case SDLK_ESCAPE: return Input::menu;
    case SDLK_D: return Input::dither;
    case SDLK_F3: return Input::textSize;
    case SDLK_B: return Input::bitDepth;
    case SDLK_M: return Input::movePattern;
    default: return std::nullopt;
    }
}

std::optional<std::size_t> numberKey(SDL_Keycode key)
{
    if (key >= SDLK_1 && key <= SDLK_9) {
        return key - SDLK_1;
    }
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9) {
        return key - SDLK_KP_1;
    }
    return std::nullopt;
}

} // namespace

GuideInput::GuideInput(SDL_Window* window, guide::Session& session)
    : window_(window), session_(session), originalCursorVisibility_(SDL_CursorVisible())
{
    syncCursor(Clock::now());
}

GuideInput::~GuideInput()
{
    if (originalCursorVisibility_) {
        SDL_ShowCursor();
    } else {
        SDL_HideCursor();
    }
}

ui::Point GuideInput::toPixels(float x, float y) const
{
    int width = 0;
    int height = 0;
    int pixelsWide = 0;
    int pixelsHigh = 0;
    if (!SDL_GetWindowSize(window_, &width, &height) ||
        !SDL_GetWindowSizeInPixels(window_, &pixelsWide, &pixelsHigh) || width <= 0 || height <= 0) {
        return {-1, -1};
    }
    return {x * static_cast<float>(pixelsWide) / static_cast<float>(width),
            y * static_cast<float>(pixelsHigh) / static_cast<float>(height)};
}

std::optional<ui::Rect> GuideInput::dividerRegion() const
{
    for (const auto& target : targets_) {
        if (target.action.command == Command::divider) return target.bounds;
    }
    return std::nullopt;
}

bool GuideInput::activate(Action action)
{
    switch (action.command) {
    case Command::back:
    case Command::next: {
        if (session_.menuOpen()) {
            if (action.command == Command::back) {
                invalidateTargets();
                return session_.handle(Input::menu);
            }
            return false;
        }
        const bool back = action.command == Command::back;
        if (back ? session_.canGoBack() : session_.canGoNext()) {
            invalidateTargets();
            return session_.handle(back ? Input::previous : Input::next);
        }
        return false;
    }
    case Command::menu:
        invalidateTargets();
        return session_.handle(Input::menu);
    case Command::answer:
        session_.selectAnswer(action.index);
        return false;
    case Command::setting:
        session_.cycleSetting(action.index);
        invalidateTargets();
        return false;
    case Command::dither:
    case Command::bitDepth:
    case Command::movePattern:
        invalidateTargets();
        return session_.handle(action.command == Command::dither ? Input::dither :
            action.command == Command::bitDepth ? Input::bitDepth : Input::movePattern);
    case Command::divider:
    case Command::probeRegion:
        return false;
    case Command::menuChoice:
        invalidateTargets();
        return session_.activateMenuChoice(action.index);
    }
    return false;
}

bool GuideInput::key(const SDL_KeyboardEvent& event, bool& running)
{
    if (event.key == SDLK_TAB) {
        pointer_.tab(targets_, (event.mod & SDL_KMOD_SHIFT) != 0);
        return true;
    }
    if (const auto number = numberKey(event.key);
        number && !session_.menuOpen() && session_.canAnswer() && *number < session_.screen().answers.size()) {
        session_.selectAnswer(*number);
        pointer_.focus(Action{Command::answer, *number});
        return true;
    }
    const auto input = navigationKey(event.key);
    if (!input) {
        return false;
    }
    if (*input == Input::confirm) {
        if (const auto focused = pointer_.tabFocus()) {
            pointer_.focus(focused);
            running = !activate(*focused);
            return true;
        }
    }

    const bool menuWasOpen = session_.menuOpen();
    const auto& previousScreen = session_.screen();
    running = !session_.handle(*input);
    if (&previousScreen != &session_.screen() || menuWasOpen != session_.menuOpen() ||
        *input == Input::textSize || *input == Input::dither ||
        *input == Input::bitDepth || *input == Input::movePattern) {
        invalidateTargets();
    }
    if (session_.menuOpen()) {
        pointer_.focus(Action{Command::menuChoice, session_.menuChoice()});
    } else if (*input == Input::up || *input == Input::down) {
        const auto answer = session_.answer();
        pointer_.focus(answer ? std::optional{Action{Command::answer, *answer}} : std::nullopt);
    } else if (!menuWasOpen && (*input == Input::previous || *input == Input::next ||
                                *input == Input::confirm)) {
        pointer_.focus(Action{*input == Input::previous ? Command::back : Command::next});
    } else {
        pointer_.focus(std::nullopt);
    }
    return true;
}

bool GuideInput::mouse(const SDL_Event& event, bool licensesOpen, bool& running)
{
    switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION: {
        if (event.motion.windowID != SDL_GetWindowID(window_)) {
            return false;
        }
        const auto oldHover = pointer_.hovered(targets_);
        const bool keyboard = pointer_.keyboardFocus().has_value();
        if (event.motion.xrel != 0.0f || event.motion.yrel != 0.0f) {
            timer_.motion(Clock::now());
        }
        const auto point = toPixels(event.motion.x, event.motion.y);
        pointer_.move(point);
        const auto region = dividerRegion();
        const bool dragged = region && session_.divider().drag(*region, point, ui::dividerMinimumPane);
        return dragged || keyboard || oldHover != pointer_.hovered(targets_);
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (licensesOpen || event.button.windowID != SDL_GetWindowID(window_)) {
            return false;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT) {
            pointer_.focus(std::nullopt);
            running = !activate({Command::back});
            return true;
        }
        if (event.button.button != SDL_BUTTON_LEFT) {
            return false;
        }
        const auto position = toPixels(event.button.x, event.button.y);
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            const auto region = dividerRegion();
            if (region && position.y >= region->y - ui::dividerHandleRadius &&
                session_.divider().begin(*region, {position.x, std::max(position.y, static_cast<float>(region->y))},
                                         ui::dividerHandleRadius, ui::dividerMinimumPane)) {
                pointer_.cancel();
                return true;
            }
            pointer_.press(targets_, position);
        } else if (session_.divider().dragging()) {
            session_.divider().end();
        } else if (const auto action = pointer_.release(targets_, position)) {
            running = !activate(*action);
        }
        return true;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        if (licensesOpen || session_.menuOpen() || event.wheel.windowID != SDL_GetWindowID(window_) ||
            event.wheel.integer_y == 0) {
            return false;
        }
        const bool up = (event.wheel.integer_y > 0) != (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED);
        pointer_.focus(std::nullopt);
        running = !activate({up ? Command::back : Command::next});
        return true;
    }
    default:
        return false;
    }
}

bool GuideInput::syncCursor(Clock::time_point now)
{
    const bool inWindow = SDL_GetMouseFocus() == window_;
    const bool visible = timer_.visible(now, session_.keepsCursorVisible());
    const bool changed = pointerVisible_ != visible;
    pointerVisible_ = visible;
    const bool showSystemCursor = !inWindow || visible;
    if (showSystemCursor != SDL_CursorVisible()) {
        if (showSystemCursor) {
            SDL_ShowCursor();
        } else {
            SDL_HideCursor();
        }
    }
    return changed;
}

int GuideInput::waitTimeout(int otherwise, Clock::time_point now) const
{
    const auto hideAt = timer_.deadline();
    if (!pointerVisible_ || session_.keepsCursorVisible() || !hideAt) {
        return otherwise;
    }
    return static_cast<int>(std::clamp(std::chrono::ceil<std::chrono::milliseconds>(*hideAt - now).count(),
                                       static_cast<std::chrono::milliseconds::rep>(0),
                                       static_cast<std::chrono::milliseconds::rep>(otherwise)));
}

void GuideInput::leave()
{
    timer_.reset();
    pointer_.leave();
    session_.divider().end();
}

} // namespace calib::app
