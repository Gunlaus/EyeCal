#include "patterns/ramp.hpp"
#include "ramp_frag.hpp"

#include <array>
#include <cstddef>

namespace calib::patterns {
namespace {

struct RampPush {
    std::array<std::int32_t, 4> rect;
    std::array<std::int32_t, 2> viewport;
    std::uint32_t bits;
    std::uint32_t dither;
    std::uint32_t coarse;
    std::uint32_t pattern;
    std::uint32_t target;
    float phase;
};
static_assert(offsetof(RampPush, bits) == 24);
static_assert(sizeof(RampPush) == 48);

} // namespace

vk::QuadPipeline createRamp(VkDevice device, VkFormat format)
{
    return vk::createQuadPipeline(device, format, ramp_frag, sizeof(RampPush), false);
}

void drawRamp(VkCommandBuffer command, vk::QuadPipeline pipeline, VkExtent2D extent,
              VkRect2D rect, OutputDepth depth, Dither mode, bool coarse,
              guide::Pattern pattern, std::uint32_t target, float phase)
{
    if (rect.extent.width == 0 || rect.extent.height == 0) {
        return;
    }
    const RampPush push{
        {rect.offset.x, rect.offset.y,
         static_cast<std::int32_t>(rect.extent.width), static_cast<std::int32_t>(rect.extent.height)},
        {static_cast<std::int32_t>(extent.width), static_cast<std::int32_t>(extent.height)},
        static_cast<std::uint32_t>(depth), static_cast<std::uint32_t>(mode), coarse ? 1u : 0u,
        static_cast<std::uint32_t>(pattern), target, phase
    };
    vk::bindQuadPipeline(command, pipeline, extent, rect);
    vkCmdPushConstants(command, pipeline.layout, vk::quadStages, 0, sizeof(push), &push);
    vkCmdDraw(command, 6, 1, 0, 0);
}

} // namespace calib::patterns
