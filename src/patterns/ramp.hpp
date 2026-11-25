#pragma once

#include "patterns/quantize.hpp"
#include "vk/quad.hpp"

namespace calib::patterns {

vk::QuadPipeline createRamp(VkDevice device, VkFormat format);
void drawRamp(VkCommandBuffer command, vk::QuadPipeline pipeline, VkExtent2D extent,
              VkRect2D rect, OutputDepth depth, Dither mode, bool coarse);

} // namespace calib::patterns
