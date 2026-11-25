#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <span>

namespace calib::ui {

struct Point {
    float x;
    float y;
};

struct Rect {
    int x;
    int y;
    int width;
    int height;
    bool contains(Point point) const;
};

enum class Command { back, next, menu, answer, menuChoice, setting, dither, bitDepth, movePattern, startSuite, divider, probeRegion };

struct Action {
    Command command;
    std::size_t index = 0;
    bool operator==(const Action&) const = default;
};

struct HitTarget {
    Rect bounds;
    Action action;
    bool enabled = true;
};

std::optional<Action> hitTest(std::span<const HitTarget> targets, Point point);

class CursorTimer {
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto idleDelay = std::chrono::seconds(2);

    void motion(Clock::time_point now) { lastMotion_ = now; }
    void reset() { lastMotion_.reset(); }
    bool visible(Clock::time_point now, bool pinned) const;
    std::optional<Clock::time_point> deadline() const;

private:
    std::optional<Clock::time_point> lastMotion_;
};

class Pointer {
public:
    void move(Point point) { position_ = point; keyboardFocus_.reset(); tabFocus_ = false; }
    void press(std::span<const HitTarget> targets, Point point);
    std::optional<Action> release(std::span<const HitTarget> targets, Point point);
    void cancel() { pressed_.reset(); }
    void leave() { position_.reset(); cancel(); }
    std::optional<Action> hovered(std::span<const HitTarget> targets) const;
    std::optional<Action> pressed() const { return pressed_; }
    std::optional<Action> keyboardFocus() const { return keyboardFocus_; }
    std::optional<Action> tabFocus() const { return tabFocus_ ? keyboardFocus_ : std::nullopt; }
    void focus(std::optional<Action> action);
    void tab(std::span<const HitTarget> targets, bool backwards);

private:
    std::optional<Point> position_;
    std::optional<Action> pressed_;
    std::optional<Action> keyboardFocus_;
    bool tabFocus_ = false;
};

inline constexpr int dividerHandleRadius = 12;
inline constexpr int dividerMinimumPane = 24;

enum class Axis { x, y };

// An axis-aligned handle with a pixel hit radius and a minimum pane size on each side.
// The stored split is relative to the current bounds, so resizing preserves the ratio.
class Divider {
public:
    explicit Divider(Axis axis) : axis_(axis) {}
    float coordinate(Rect bounds, float minPane) const;
    bool begin(Rect bounds, Point point, float hitRadius, float minPane);
    bool drag(Rect bounds, Point point, float minPane);
    void end() { grabOffset_.reset(); }
    bool dragging() const { return grabOffset_.has_value(); }

private:
    Axis axis_;
    float split_ = 0.5f;
    std::optional<float> grabOffset_;
};

} // namespace calib::ui
