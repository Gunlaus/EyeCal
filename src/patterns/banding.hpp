#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>

namespace calib::patterns {

struct Observation {
    std::uint32_t display;
    int width;
    int height;
    bool exclusive;
    bool operator==(const Observation&) const = default;
};

enum class ProbeResult { pending, unavailable, borderless, smoother, likelyTruncation, inconclusive };

class BitDepthProbe {
public:
    bool observe(bool supportsTenBit, bool activeTenBit, Observation observation);
    bool reset();
    ProbeResult result(std::optional<std::size_t> answer) const;
    bool ready() const { return eightBit_.has_value() && tenBit_.has_value(); }
    bool available() const { return supported_.value_or(true); }

private:
    std::optional<bool> supported_;
    std::optional<Observation> eightBit_;
    std::optional<Observation> tenBit_;
};

float bandingRamp(std::uint32_t column, std::uint32_t width, bool midGrey, float phase = 0.0f);
unsigned countRampSteps(std::uint32_t width, bool tenBit);

} // namespace calib::patterns
