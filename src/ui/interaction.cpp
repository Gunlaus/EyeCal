#include "ui/interaction.hpp"

#include <algorithm>
#include <cmath>

namespace calib::ui {

bool Rect::contains(Point point) const
{
    return width > 0 && height > 0 && point.x >= static_cast<float>(x) &&
           point.y >= static_cast<float>(y) && point.x < static_cast<float>(x + width) &&
           point.y < static_cast<float>(y + height);
}

std::optional<Action> hitTest(std::span<const HitTarget> targets, Point point)
{
    for (auto it = targets.rbegin(); it != targets.rend(); ++it) {
        if (it->bounds.contains(point)) {
            return it->enabled ? std::optional{it->action} : std::nullopt;
        }
    }
    return std::nullopt;
}

bool CursorTimer::visible(Clock::time_point now, bool pinned) const
{
    const auto hideAt = deadline();
    return pinned || (hideAt && now < *hideAt);
}

std::optional<CursorTimer::Clock::time_point> CursorTimer::deadline() const
{
    return lastMotion_ ? std::optional{*lastMotion_ + idleDelay} : std::nullopt;
}

void Pointer::press(std::span<const HitTarget> targets, Point point)
{
    move(point);
    pressed_ = hitTest(targets, point);
}

std::optional<Action> Pointer::release(std::span<const HitTarget> targets, Point point)
{
    move(point);
    const auto target = hitTest(targets, point);
    const auto action = target == pressed_ ? target : std::nullopt;
    cancel();
    return action;
}

std::optional<Action> Pointer::hovered(std::span<const HitTarget> targets) const
{
    return position_ ? hitTest(targets, *position_) : std::nullopt;
}

void Pointer::focus(std::optional<Action> action)
{
    keyboardFocus_ = action;
    tabFocus_ = false;
    position_.reset();
    cancel();
}

void Pointer::tab(std::span<const HitTarget> targets, bool backwards)
{
    const auto current = keyboardFocus_;
    focus(std::nullopt);
    std::optional<Action> edge;
    bool advance = !current;
    for (std::size_t i = 0; i < targets.size(); ++i) {
        const auto& target = targets[backwards ? targets.size() - 1 - i : i];
        if (!target.enabled) {
            continue;
        }
        if (!edge) {
            edge = target.action;
        }
        if (advance) {
            keyboardFocus_ = target.action;
            break;
        }
        advance = target.action == current;
    }
    if (!keyboardFocus_) {
        keyboardFocus_ = edge;
    }
    tabFocus_ = keyboardFocus_.has_value();
}

namespace {

struct Rail {
    float start;
    float length;
    float minPane;

    float clamp(float value) const
    {
        return std::clamp(value, start + minPane, start + length - minPane);
    }
};

Rail rail(Rect bounds, Axis axis, float minPane)
{
    const int start = axis == Axis::x ? bounds.x : bounds.y;
    const auto length = static_cast<float>(std::max(0, axis == Axis::x ? bounds.width : bounds.height));
    return {static_cast<float>(start), length, std::clamp(minPane, 0.0f, length / 2.0f)};
}

float along(Point point, Axis axis)
{
    return axis == Axis::x ? point.x : point.y;
}

} // namespace

float Divider::coordinate(Rect bounds, float minPane) const
{
    const auto track = rail(bounds, axis_, minPane);
    return track.clamp(track.start + track.length * split_);
}

bool Divider::begin(Rect bounds, Point point, float hitRadius, float minPane)
{
    end();
    const auto offset = along(point, axis_) - coordinate(bounds, minPane);
    if (bounds.contains(point) && std::abs(offset) <= hitRadius) {
        grabOffset_ = offset;
    }
    return dragging();
}

bool Divider::drag(Rect bounds, Point point, float minPane)
{
    const auto track = rail(bounds, axis_, minPane);
    const auto position = along(point, axis_);
    if (!grabOffset_ || track.length <= 2 * track.minPane || !std::isfinite(position)) {
        return false;
    }
    const auto next = (track.clamp(position - *grabOffset_) - track.start) / track.length;
    if (next == split_) {
        return false;
    }
    split_ = next;
    return true;
}

} // namespace calib::ui
