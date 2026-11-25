#pragma once

#include "guide/guide.hpp"
#include "ui/interaction.hpp"

#include <SDL3/SDL.h>
#include <vector>

namespace calib::app {

class GuideInput {
public:
    GuideInput(SDL_Window* window, guide::Session& session);
    ~GuideInput();
    GuideInput(const GuideInput&) = delete;
    GuideInput& operator=(const GuideInput&) = delete;

    bool key(const SDL_KeyboardEvent& event, bool& running);
    bool mouse(const SDL_Event& event, bool licensesOpen, bool& running);
    bool syncCursor(ui::CursorTimer::Clock::time_point now);
    int waitTimeout(int otherwise, ui::CursorTimer::Clock::time_point now) const;
    void invalidateTargets() { targets_.clear(); pointer_.cancel(); session_.divider().end(); }
    void leave();

    const ui::Pointer& pointer() const { return pointer_; }
    bool pointerVisible() const { return pointerVisible_; }
    std::vector<ui::HitTarget>& targets() { return targets_; }

private:
    bool activate(ui::Action action);
    std::optional<ui::Rect> dividerRegion() const;
    ui::Point toPixels(float x, float y) const;

    SDL_Window* window_;
    guide::Session& session_;
    std::vector<ui::HitTarget> targets_;
    ui::Pointer pointer_;
    ui::CursorTimer timer_;
    bool originalCursorVisibility_;
    bool pointerVisible_ = false;
};

} // namespace calib::app
