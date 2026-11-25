#include "app/guide_input.hpp"
#include "platform/platform.hpp"
#include "vk/renderer.hpp"
#include "licenses.hpp"
#include "locale.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <vector>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

long pixelSize(int size, float density)
{
    return std::lround(static_cast<float>(size) * density);
}

using Clock = std::chrono::steady_clock;

void requireSDL(bool success, std::string_view operation)
{
    if (!success) {
        throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }
}

struct Sdl {
    ~Sdl() { SDL_Quit(); }
};

struct Loader {
    ~Loader() { SDL_Vulkan_UnloadLibrary(); }
};

bool setExclusiveDisplayMode(SDL_Window* window)
{
    const auto display = SDL_GetDisplayForWindow(window);
    const auto* desktop = SDL_GetDesktopDisplayMode(display);
    SDL_DisplayMode mode{};
    if (!desktop || !SDL_GetClosestFullscreenDisplayMode(
            display, desktop->w, desktop->h, desktop->refresh_rate, true, &mode)) {
        return false;
    }
    if (pixelSize(desktop->w, desktop->pixel_density) != pixelSize(mode.w, mode.pixel_density) ||
        pixelSize(desktop->h, desktop->pixel_density) != pixelSize(mode.h, mode.pixel_density)) {
        return false;
    }
    return SDL_SetWindowFullscreenMode(window, &mode) && SDL_SyncWindow(window);
}

class LicenseView {
public:
    LicenseView()
    {
        auto text = calib::ui::licenseText;
        while (!text.empty()) {
            const auto length = text.find('\n');
            const auto line = text.substr(0, length);
            std::string ascii;
            for (std::size_t index = 0; index < line.size(); ++index) {
                const auto character = static_cast<unsigned char>(line[index]);
                if (character == '\r') {
                    continue;
                }
                if (line.substr(index).starts_with("©")) {
                    ascii += "(c)";
                    ++index;
                } else if (character < 128) {
                    ascii += static_cast<char>(character);
                } else if ((character & 0xc0) != 0x80) {
                    ascii += '?';
                }
            }
            constexpr std::size_t columns = 48;
            auto remaining = std::string_view(ascii);
            while (remaining.size() > columns) {
                const auto boundary = remaining.rfind(' ', columns);
                const auto split = boundary == 0 || boundary == std::string_view::npos
                    ? columns : boundary;
                lines_.emplace_back(remaining.substr(0, split));
                remaining.remove_prefix(split);
                if (remaining.starts_with(' ')) {
                    remaining.remove_prefix(1);
                }
            }
            lines_.emplace_back(remaining);
            if (length == std::string_view::npos) {
                break;
            }
            text.remove_prefix(length + 1);
        }
    }

    bool shown() const { return shown_; }
    void toggle() { shown_ = !shown_; }

    void move(bool forward)
    {
        if (!shown_) {
            return;
        }
        if (forward && first_ + linesPerPage < lines_.size()) {
            first_ += linesPerPage;
        } else if (!forward && first_ >= linesPerPage) {
            first_ -= linesPerPage;
        }
    }

    std::span<const std::string_view> page()
    {
        if (!shown_) {
            return {};
        }
        page_.clear();
        page_.push_back("Third-party licenses   PgUp/PgDn   F2 Close");
        page_.push_back("");
        for (auto index = first_; index < std::min(first_ + linesPerPage, lines_.size()); ++index) {
            page_.push_back(lines_[index]);
        }
        return page_;
    }

private:
    static constexpr std::size_t linesPerPage = 16;
    std::vector<std::string> lines_;
    std::vector<std::string_view> page_;
    std::size_t first_ = 0;
    bool shown_ = false;
};

struct ChromeState {
    std::size_t step;
    bool menu;
    std::optional<calib::ui::Action> hover;
    std::optional<calib::ui::Action> pressed;
    std::optional<calib::ui::Action> focus;
    bool operator==(const ChromeState&) const = default;
};

int run()
{
    const auto content = calib::guide::loadContent(calib::guide::englishJson);
    calib::guide::Session guide(content);
    requireSDL(SDL_Init(SDL_INIT_VIDEO), "Start SDL");
    const Sdl sdl{};
    requireSDL(SDL_Vulkan_LoadLibrary(nullptr), "Load the Vulkan driver");
    const Loader loader{};
    const std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{
        SDL_CreateWindow("EyeCal", 960, 540,
                         SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN),
        SDL_DestroyWindow
    };
    requireSDL(window != nullptr, "Open borderless fullscreen window");
    requireSDL(SDL_SyncWindow(window.get()), "Settle initial fullscreen window");

    calib::vk::Renderer renderer;
    renderer.initialize(window.get());
    calib::app::GuideInput input(window.get(), guide);
    LicenseView licenses;
    bool diagnostics = false;
    bool exclusiveMode = false;
    bool running = true;
    bool redraw = true;
    std::string_view notice;
    auto outputColor = calib::platform::outputColor(window.get());
    auto nextColorCheck = Clock::now() + std::chrono::seconds(2);
    std::optional<Clock::time_point> settlingSince;
    std::optional<ChromeState> chrome;
    auto chromeStart = Clock::now();
    bool animatingChrome = false;

    while (running) {
        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, input.waitTimeout((animatingChrome || (guide.movingPattern() && !licenses.shown())) ? 16 : redraw ? 35 : 200, Clock::now()))) {
            do {
                switch (event.type) {
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    running = false;
                    break;
                case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                    input.leave();
                    redraw = true;
                    break;
                case SDL_EVENT_WINDOW_MOUSE_ENTER:
                    redraw = true;
                    break;
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    input.leave();
                    [[fallthrough]];
                case SDL_EVENT_WINDOW_RESTORED:
                case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
                case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
                case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
                case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
                    guide.resetProbe();
                    [[fallthrough]];
                case SDL_EVENT_WINDOW_EXPOSED:
                    input.invalidateTargets();
                    renderer.invalidate();
                    nextColorCheck = Clock::now();
                    redraw = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.repeat) {
                        break;
                    }
                    switch (event.key.key) {
                    case SDLK_F1:
                        input.invalidateTargets();
                        diagnostics = !diagnostics;
                        redraw = true;
                        break;
                    case SDLK_F2:
                        input.invalidateTargets();
                        licenses.toggle();
                        redraw = true;
                        break;
                    case SDLK_PAGEUP:
                    case SDLK_PAGEDOWN:
                        licenses.move(event.key.key == SDLK_PAGEDOWN);
                        redraw = true;
                        break;
                    case SDLK_F11:
                        guide.resetProbe();
                        input.invalidateTargets();
                        renderer.dropSwapchain();
                        notice = {};
                        if (!exclusiveMode && setExclusiveDisplayMode(window.get())) {
                            exclusiveMode = true;
                        } else {
                            if (!exclusiveMode) {
                                notice = "Matching exclusive display mode unavailable.";
                            }
                            exclusiveMode = false;
                            requireSDL(SDL_SetWindowFullscreenMode(window.get(), nullptr) &&
                                       SDL_SyncWindow(window.get()), "Return to borderless fullscreen");
                        }
                        redraw = true;
                        break;
                    default:
                        if (licenses.shown() && event.key.key == SDLK_ESCAPE) {
                            licenses.toggle();
                            redraw = true;
                        } else if (!licenses.shown()) {
                            redraw = input.key(event.key, running) || redraw;
                        }
                        break;
                    }
                    break;
                default:
                    redraw = input.mouse(event, licenses.shown(), running) || redraw;
                    break;
                }
            } while (running && SDL_PollEvent(&event));
        }

        const auto now = Clock::now();
        redraw = input.syncCursor(now) || redraw;
        if (now >= nextColorCheck) {
            const auto current = calib::platform::outputColor(window.get());
            redraw = redraw || current != outputColor;
            if (current != outputColor) {
                guide.resetProbe();
                input.invalidateTargets();
            }
            outputColor = current;
            nextColorCheck = now + std::chrono::seconds(2);
        }
        const ChromeState currentChrome{
            guide.step(), guide.menuOpen(),
            input.pointerVisible() ? input.pointer().hovered(input.targets()) : std::nullopt,
            input.pointer().pressed(), input.pointer().keyboardFocus()
        };
        if (currentChrome != chrome) {
            chrome = currentChrome;
            chromeStart = now;
        }
        const float t = std::clamp(std::chrono::duration<float, std::milli>(now - chromeStart).count() /
                                  calib::ui::theme::transitionMs, 0.0f, 1.0f);
        const bool motionAllowed = !guide.reduceMotion() && !licenses.shown() &&
            (guide.menuOpen() || guide.screen().pattern == calib::guide::Pattern::none);
        const float motion = motionAllowed ? (1.0f - t) * (1.0f - t) * (1.0f - t) : 0.0f;
        redraw = redraw || animatingChrome || (motionAllowed && t < 1.0f);
        animatingChrome = motionAllowed && t < 1.0f;
        const bool moving = guide.movingPattern() && !licenses.shown();
        const float patternPhase = moving ? static_cast<float>(std::fmod(
            std::chrono::duration<double>(now.time_since_epoch()).count(), 16.0) / 16.0) : 0.0f;
        if (running && (redraw || moving)) {
            const bool focused = (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_INPUT_FOCUS) != 0;
            const bool presented = renderer.draw({guide, input.pointer(), input.pointerVisible(),
                                                  motion, patternPhase, diagnostics, exclusiveMode,
                                                  focused, outputColor, notice, licenses.page()},
                                                 input.targets());
            redraw = !presented;
            if (!presented) {
                guide.resetProbe();
            } else if (guide.screen().pattern == calib::guide::Pattern::bandingDepth &&
                       !guide.menuOpen() && !licenses.shown() && focused &&
                       (outputColor != calib::platform::OutputColor::hdr || !renderer.supportsTenBit())) {
                const auto display = SDL_GetDisplayForWindow(window.get());
                for (const auto& target : input.targets()) {
                    if (target.action.command == calib::ui::Command::probeRegion && display != 0) {
                        redraw = guide.probe().observe(renderer.supportsTenBit(), renderer.activeTenBit(),
                            {display, target.bounds.width, target.bounds.height, renderer.exclusiveGranted()});
                        break;
                    }
                }
            }
            if (presented || !focused || (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED)) {
                settlingSince.reset();
            } else if (!settlingSince) {
                settlingSince = now;
            } else if (now - *settlingSince >= std::chrono::seconds(5)) {
                throw std::runtime_error("Fullscreen presentation did not settle. "
                                         "Set your display to its recommended resolution and restart.");
            }
        }
    }
    return 0;
}

} // namespace

int main(int, char**)
{
    try {
        return run();
    } catch (const std::exception& error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", error.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "EyeCal", error.what(), nullptr);
        return 1;
    }
}
