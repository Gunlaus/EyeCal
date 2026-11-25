#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "guide/guide.hpp"
#include "locale.hpp"
#include "ui/interaction.hpp"

#include <array>

using namespace calib::ui;

TEST_CASE("Button hit testing uses half-open bounds, visible stacking and disabled targets")
{
    constexpr Action back{Command::back};
    constexpr Action next{Command::next};
    constexpr Action menu{Command::menu};
    const std::array targets{
        HitTarget{{12, 40, 48, 32}, back},
        HitTarget{{60, 40, 48, 32}, next, false},
        HitTarget{{36, 40, 24, 16}, menu}
    };
    CHECK(hitTest(targets, {12, 40}) == back);
    CHECK(hitTest(targets, {35.9f, 55}) == back);
    CHECK(hitTest(targets, {36, 55}) == menu);
    CHECK(hitTest(targets, {36, 56}) == back);
    CHECK_FALSE(hitTest(targets, {60, 56}));
    CHECK_FALSE(hitTest(targets, {11.9f, 55}));
    CHECK_FALSE(hitTest(targets, {12, 72}));
    CHECK_FALSE(hitTest(targets, {30, 140}));

    const std::array blocked{targets[0], HitTarget{{12, 40, 12, 12}, menu, false}};
    CHECK_FALSE(hitTest(blocked, {16, 45}));
}

TEST_CASE("Click activation requires the same enabled target on press and release")
{
    constexpr Action first{Command::answer, 0};
    constexpr Action second{Command::answer, 1};
    const std::array targets{HitTarget{{0, 0, 30, 30}, first},
                             HitTarget{{30, 0, 30, 30}, second}};
    Pointer pointer;
    pointer.move({5, 5});
    CHECK(pointer.hovered(targets) == first);
    CHECK_FALSE(pointer.release(targets, {5, 5}));

    pointer.press(targets, {5, 5});
    CHECK(pointer.pressed() == first);
    pointer.move({35, 5});
    CHECK(pointer.hovered(targets) == second);
    CHECK_FALSE(pointer.release(targets, {35, 5}));
    CHECK_FALSE(pointer.pressed());

    pointer.press(targets, {5, 5});
    pointer.move({65, 5});
    CHECK(pointer.release(targets, {5, 5}) == first);
    pointer.press(targets, {5, 5});
    pointer.leave();
    CHECK_FALSE(pointer.hovered(targets));
    CHECK_FALSE(pointer.release(targets, {5, 5}));
}

TEST_CASE("Tab and Shift+Tab wrap around disabled controls and mouse motion clears focus")
{
    constexpr Action back{Command::back};
    constexpr Action next{Command::next};
    constexpr Action menu{Command::menu};
    const std::array targets{HitTarget{{0, 0, 30, 30}, back, false},
                             HitTarget{{30, 0, 30, 30}, next},
                             HitTarget{{60, 0, 30, 30}, menu}};
    Pointer pointer;
    pointer.focus(next);
    CHECK(pointer.keyboardFocus() == next);
    CHECK_FALSE(pointer.tabFocus());
    pointer.tab(targets, false);
    CHECK(pointer.tabFocus() == menu);
    pointer.tab(targets, false);
    CHECK(pointer.tabFocus() == next);
    pointer.tab(targets, true);
    CHECK(pointer.tabFocus() == menu);
    pointer.move({5, 10});
    CHECK_FALSE(pointer.tabFocus());
    CHECK_FALSE(pointer.keyboardFocus());
}

TEST_CASE("Cursor starts hidden and expires exactly two seconds after the latest motion")
{
    using namespace std::chrono_literals;
    CursorTimer timer;
    const CursorTimer::Clock::time_point start{};
    CHECK_FALSE(timer.visible(start, false));
    CHECK_FALSE(timer.deadline());
    CHECK(timer.visible(start, true));
    timer.motion(start);
    CHECK(timer.deadline() == start + 2s);
    CHECK(timer.visible(start + 1999ms, false));
    CHECK_FALSE(timer.visible(start + 2s, false));

    timer.motion(start + 1500ms);
    CHECK(timer.visible(start + 2s, false));
    CHECK(timer.deadline() == start + 3500ms);
    CHECK_FALSE(timer.visible(start + 3500ms, false));
    CHECK(timer.visible(start + 10s, true));
    timer.reset();
    CHECK_FALSE(timer.visible(start + 11s, false));
    CHECK(timer.visible(start + 11s, true));
}

TEST_CASE("Generic divider preserves grab offset, clamps panes and works on both axes")
{
    constexpr Rect bounds{20, 60, 200, 100};
    Divider divider(Axis::x);
    CHECK(divider.coordinate(bounds, 24) == doctest::Approx(120));
    CHECK_FALSE(divider.begin(bounds, {80, 70}, 6, 24));
    CHECK_FALSE(divider.begin(bounds, {120, 10}, 6, 24));
    CHECK(divider.begin(bounds, {124, 70}, 6, 24));
    CHECK(divider.drag(bounds, {164, 70}, 24));
    CHECK(divider.coordinate(bounds, 24) == doctest::Approx(160));
    CHECK(divider.coordinate({40, 60, 400, 100}, 24) == doctest::Approx(320));
    CHECK(divider.drag(bounds, {500, 70}, 24));
    CHECK(divider.coordinate(bounds, 24) == doctest::Approx(196));
    CHECK(divider.drag(bounds, {-500, 70}, 24));
    CHECK(divider.coordinate(bounds, 24) == doctest::Approx(44));
    divider.end();
    CHECK_FALSE(divider.drag(bounds, {120, 70}, 24));

    Divider vertical(Axis::y);
    CHECK(vertical.begin(bounds, {40, 110}, 5, 10));
    CHECK(vertical.drag(bounds, {40, 135}, 10));
    CHECK(vertical.coordinate(bounds, 10) == doctest::Approx(135));
    CHECK_FALSE(vertical.drag(bounds, {40, 135}, 100));
}

TEST_CASE("Direct answer selection, cursor pinning and disabled navigation follow the guide")
{
    using namespace calib::guide;
    auto content = loadContent(englishJson);
    content.screens.resize(2);
    content.screens.front().question = "Test question";
    content.screens.front().answers = {"One", "Two"};
    content.screens.back().id = "summary";
    Session session(content);
    CHECK_FALSE(session.canGoBack());
    CHECK(session.canGoNext());
    CHECK_FALSE(session.keepsCursorVisible());
    session.selectAnswer(1);
    CHECK(session.answer() == 1);
    session.selectAnswer(99);
    CHECK(session.answer() == 1);

    CHECK_FALSE(session.handle(Input::menu));
    CHECK(session.keepsCursorVisible());
    session.selectAnswer(0);
    CHECK(session.answer() == 1);
    CHECK_FALSE(session.activateMenuChoice(99));
    CHECK(session.menuOpen());
    CHECK_FALSE(session.activateMenuChoice(0));
    CHECK_FALSE(session.menuOpen());
    CHECK_FALSE(session.handle(Input::next));
    CHECK(session.canGoBack());
    CHECK_FALSE(session.canGoNext());
    CHECK(session.keepsCursorVisible());
}
