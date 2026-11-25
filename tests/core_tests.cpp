#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "guide/guide.hpp"
#include "patterns/hdr.hpp"
#include "locale.hpp"
#include "patterns/quantize.hpp"

#include <array>
#include <limits>
#include <stdexcept>
#include <string>

using namespace calib;

TEST_CASE("UNORM rounds half up and retains exact 8-bit and 10-bit codes with dither off")
{
    using namespace patterns;
    for (auto depth : {OutputDepth::unorm8, OutputDepth::unorm10}) {
        const auto maxCode = (1u << static_cast<unsigned>(depth)) - 1;
        for (unsigned code = 0; code <= maxCode; ++code) {
            CHECK(quantizeCode(static_cast<float>(code) / static_cast<float>(maxCode), depth) == code);
        }
        CHECK(quantizeCode(0.5f, depth) == (maxCode + 1) / 2);
        CHECK(quantizeCode(-1.0f, depth) == 0);
        CHECK(quantizeCode(2.0f, depth) == maxCode);
    }
    CHECK(quantizeCode(std::numeric_limits<float>::quiet_NaN(), OutputDepth::unorm8) == 0);
}

TEST_CASE("Ordered dither uses the Bayer 8x8 midpoints")
{
    using namespace patterns;
    constexpr std::array<unsigned, 8> row{0, 32, 8, 40, 2, 34, 10, 42};
    for (unsigned x = 0; x < row.size(); ++x) {
        const float expected = (static_cast<float>(row[x]) + 0.5f) / 64.0f - 0.5f;
        CHECK(ditherOffset(Dither::ordered, {x, 0}) == expected);
        CHECK(ditherOffset(Dither::ordered, {x + 8, 16}) == expected);
    }
    unsigned up = 0;
    for (unsigned y = 0; y < 8; ++y) {
        for (unsigned x = 0; x < 8; ++x) {
            up += quantizeCode(0.5f, OutputDepth::unorm8, Dither::ordered, {x, y}) == 128;
        }
    }
    CHECK(up == 32);
}

TEST_CASE("Blue-noise texture is a centered 16x16 rank permutation")
{
    using namespace patterns;
    std::array<bool, 256> seen{};
    float sum = 0.0f;
    for (unsigned y = 0; y < 16; ++y) {
        for (unsigned x = 0; x < 16; ++x) {
            const auto rank = blueNoiseRank({x, y});
            CHECK_FALSE(seen[rank]);
            seen[rank] = true;
            CHECK(blueNoiseRank({x + 16, y + 32}) == rank);
            sum += ditherOffset(Dither::blueNoise, {x, y});
        }
    }
    CHECK(sum == 0.0f);
}

TEST_CASE("TPDF is bounded, stable for a fixed seed, and concentrated near zero")
{
    using namespace patterns;
    float sum = 0.0f;
    int center = 0;
    int tail = 0;
    for (unsigned x = 0; x < 4096; ++x) {
        const auto noise = ditherOffset(Dither::tpdf, {x, 7}, 4);
        CHECK(noise > -1.0f);
        CHECK(noise < 1.0f);
        CHECK(noise == ditherOffset(Dither::tpdf, {x, 7}, 4));
        sum += noise;
        center += noise >= -0.25f && noise <= 0.25f;
        tail += noise < -0.5f || noise > 0.5f;
    }
    CHECK(center > tail);
    CHECK(sum / 4096.0f == doctest::Approx(0.0f).epsilon(0.02));
    CHECK(ditherOffset(Dither::tpdf, {3, 9}, 0) != ditherOffset(Dither::tpdf, {3, 9}, 1));
}

TEST_CASE("Dither preserves exact endpoints and ramp spans every code in either depth")
{
    using namespace patterns;
    for (auto mode : {Dither::off, Dither::ordered, Dither::blueNoise, Dither::tpdf}) {
        CHECK(quantizeCode(0.0f, OutputDepth::unorm10, mode, {5, 7}) == 0);
        CHECK(quantizeCode(1.0f, OutputDepth::unorm10, mode, {5, 7}) == 1023);
    }
    for (auto depth : {OutputDepth::unorm8, OutputDepth::unorm10}) {
        const auto width = 1u << static_cast<unsigned>(depth);
        for (unsigned x = 0; x < width; ++x) {
            CHECK(quantizeCode(rampSample(x, width), depth) == x);
        }
    }
    CHECK(rampSample(20, 1) == 0.0f);
    CHECK(rampSample(20, 6) == 1.0f);
}

TEST_CASE("Embedded English text includes the display preparation and validates input")
{
    const auto content = guide::loadContent(guide::englishJson);
    REQUIRE(content.screens.size() == 18);
    CHECK(content.screens.front().id == "intro");
    CHECK(content.screens.front().instructions.back().find("HDR and auto colour management")
          != std::string::npos);
    CHECK(content.screens[5].pattern == guide::Pattern::sharpness);
    CHECK_THROWS_AS(guide::loadContent("{"), std::runtime_error);
    CHECK_THROWS_AS(guide::loadContent("{}"), std::runtime_error);

    std::string duplicate(guide::englishJson);
    duplicate.replace(duplicate.find("\"id\": \"levels\""), 14, "\"id\": \"intro\"");
    CHECK_THROWS_WITH(guide::loadContent(duplicate), "Load embedded en.json: Invalid screen data for intro");
}

TEST_CASE("Live test navigation, dither and direct gamma setting")
{
    using namespace guide;
    const auto content = loadContent(englishJson);
    Session session(content);
    CHECK_FALSE(session.canAnswer());
    session.handle(Input::down);
    CHECK_FALSE(session.answer());
    CHECK_FALSE(session.canGoBack());
    session.startSuite(2);
    CHECK(session.screen().pattern == Pattern::hdrPeak);
    session.startSuite(1);
    CHECK(session.screen().pattern == Pattern::greyBalance);
    session.startSuite(0);
    CHECK(session.screen().pattern == Pattern::levels);
    for (int i = 0; i < 3; ++i) session.handle(Input::next);
    REQUIRE(session.screen().pattern == Pattern::gamma);
    session.cycleSetting(0, -1);
    CHECK(session.setting(Setting::gamma) == 2);
    session.handle(Input::next);
    session.handle(Input::next);
    REQUIRE(session.screen().pattern == Pattern::ramp);
    CHECK(session.dither() == patterns::Dither::off);
    session.handle(Input::dither);
    CHECK(session.baselineDitherEnabled());
    CHECK(session.dither() == patterns::Dither::blueNoise);
    session.handle(Input::dither);
    CHECK(session.dither() == patterns::Dither::off);
    session.handle(Input::next);
    CHECK(session.screen().pattern == Pattern::bandingSplit);
    CHECK(session.dither() == patterns::Dither::blueNoise);
    session.handle(Input::dither);
    CHECK(session.dither() == patterns::Dither::tpdf);
    session.handle(Input::menu);
    CHECK(session.menuOpen());
    CHECK_FALSE(session.activateMenuChoice(15));
    CHECK(session.screen().pattern == Pattern::hdrBlack);
    CHECK_FALSE(session.menuOpen());
}

TEST_CASE("HDR PQ encodes reference luminances and quantizes absolute black")
{
    using namespace patterns;
    CHECK(quantizeCode(pqEncode(0.0f), OutputDepth::unorm10) == 0u);
    CHECK(pqEncode(100.0f) == doctest::Approx(0.5080784).epsilon(0.00002));
    CHECK(pqEncode(1000.0f) == doctest::Approx(0.7518271).epsilon(0.00002));
    CHECK(pqEncode(4000.0f) == doctest::Approx(0.9025724).epsilon(0.00002));
    CHECK(pqEncode(10000.0f) == doctest::Approx(1.0));
}

TEST_CASE("Banding ramps preserve reference endpoints and expected step counts")
{
    using namespace patterns;
    CHECK(countRampSteps(0, false) == 0);
    CHECK(countRampSteps(37, true) == 37);
    CHECK(countRampSteps(1920, false) == 256);
    CHECK(countRampSteps(1920, true) == 1024);
    CHECK(bandingRamp(0, 301, false) == 0.0f);
    CHECK(bandingRamp(300, 301, false) == doctest::Approx(0.484529f));
    CHECK(bandingRamp(0, 301, true) == doctest::Approx(0.484529f));
    CHECK(bandingRamp(300, 301, true) == doctest::Approx(0.665185f));
    CHECK(bandingRamp(0, 301, false, 0.5f) == doctest::Approx(bandingRamp(300, 301, false)));
    CHECK(bandingRamp(15, 31, false, 1.0f) == bandingRamp(15, 31, false));
}

TEST_CASE("Bit-depth comparison never suggests truncation without two matching exclusive passes")
{
    using namespace patterns;
    BitDepthProbe probe;
    constexpr Observation exclusive{1, 800, 400, true};
    CHECK(probe.result(1) == ProbeResult::pending);
    CHECK(probe.observe(false, false, exclusive));
    CHECK(probe.result(1) == ProbeResult::unavailable);
    CHECK(probe.reset());
    for (int i = 0; i < 48; ++i) {
        CHECK(probe.observe(true, false, exclusive));
        CHECK(probe.result(1) == ProbeResult::pending);
        CHECK(probe.observe(true, true, {1, 800, 400, false}));
        CHECK(probe.result(1) == ProbeResult::borderless);
        CHECK(probe.observe(true, true, {1, 800, 420, true}));
        CHECK(probe.result(1) == ProbeResult::inconclusive);
        CHECK(probe.observe(true, true, exclusive));
        CHECK(probe.result(0) == ProbeResult::smoother);
        CHECK(probe.result(1) == ProbeResult::likelyTruncation);
        CHECK(probe.result(2) == ProbeResult::inconclusive);
        CHECK_FALSE(probe.observe(true, true, exclusive));
        CHECK(probe.reset());
    }
}

TEST_CASE("Banding observations offer immediate next steps and group the four parts")
{
    using namespace guide;
    const auto content = loadContent(englishJson);
    Session session(content);
    for (int i = 0; i < 8; ++i) session.handle(Input::next);
    CHECK(session.stepCount() == 15);
    CHECK(session.step() == 7);
    CHECK_FALSE(session.canAnswer());
    session.selectAnswer(1);
    CHECK_FALSE(session.answer());
    session.probe().observe(true, false, {1, 800, 400, true});
    session.probe().observe(true, true, {1, 800, 400, false});
    session.selectAnswer(1);
    CHECK(session.resultHint().starts_with("Inconclusive in borderless mode"));
    session.handle(Input::next);
    CHECK(session.step() == 7);
    session.selectAnswer(0);
    CHECK(session.resultHint().find("local dimming") != std::string_view::npos);
    session.selectAnswer(1);
    CHECK(session.resultHint().find("Confirm 10-bit GPU output") != std::string_view::npos);
}
