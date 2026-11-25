#include "patterns/banding.hpp"
#include "patterns/quantize.hpp"

#include <cmath>

namespace calib::patterns {

bool BitDepthProbe::observe(bool supportsTenBit, bool activeTenBit, Observation observation)
{
    const bool supportChanged = supported_ != supportsTenBit;
    supported_ = supportsTenBit;
    if (!supportsTenBit) {
        const bool previous = eightBit_ || tenBit_;
        eightBit_.reset();
        tenBit_.reset();
        return supportChanged || previous;
    }
    auto& current = activeTenBit ? tenBit_ : eightBit_;
    if (current == observation) {
        return supportChanged;
    }
    current = observation;
    return true;
}

bool BitDepthProbe::reset()
{
    const bool changed = supported_.has_value() || eightBit_ || tenBit_;
    supported_.reset();
    eightBit_.reset();
    tenBit_.reset();
    return changed;
}

ProbeResult BitDepthProbe::result(std::optional<std::size_t> answer) const
{
    if (!available()) return ProbeResult::unavailable;
    if (!eightBit_ || !tenBit_) return ProbeResult::pending;
    if (!eightBit_->exclusive || !tenBit_->exclusive) return ProbeResult::borderless;
    if (eightBit_->display != tenBit_->display || eightBit_->width != tenBit_->width ||
        eightBit_->height != tenBit_->height) return ProbeResult::inconclusive;
    if (!answer) return ProbeResult::pending;
    if (*answer == 0) return ProbeResult::smoother;
    if (*answer == 1) return ProbeResult::likelyTruncation;
    return ProbeResult::inconclusive;
}

float bandingRamp(std::uint32_t column, std::uint32_t width, bool midGrey, float phase)
{
    const float x = rampSample(column, width);
    const float t = 0.5f * x + phase;
    const float moved = 1.0f - std::abs(2.0f * (t - std::floor(t)) - 1.0f);
    const float linear = 0.2f * (moved + (midGrey ? 1.0f : 0.0f));
    return linear <= 0.0031308f ? 12.92f * linear :
           1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
}

unsigned countRampSteps(std::uint32_t width, bool tenBit)
{
    unsigned steps = 0;
    std::optional<std::uint32_t> previous;
    const auto depth = tenBit ? OutputDepth::unorm10 : OutputDepth::unorm8;
    for (std::uint32_t x = 0; x < width; ++x) {
        const auto code = quantizeCode(rampSample(x, width), depth);
        if (previous != code) {
            ++steps;
            previous = code;
        }
    }
    return steps;
}

} // namespace calib::patterns
