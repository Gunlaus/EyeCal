#pragma once

#include "guide/guide.hpp"
#include "patterns/quantize.hpp"
#include "vk/quad.hpp"

namespace calib::patterns {

vk::QuadPipeline createRamp(VkDevice device, VkFormat format);
void drawRamp(VkCommandBuffer command, vk::QuadPipeline pipeline, VkExtent2D extent,
              VkRect2D rect, OutputDepth depth, Dither mode, bool coarse,
              guide::Pattern pattern = guide::Pattern::ramp, std::uint32_t target = 0,
              float phase = 0.0f);

} // namespace calib::patterns
