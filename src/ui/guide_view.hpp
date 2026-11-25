#pragma once

#include "guide/guide.hpp"
#include "ui/interaction.hpp"
#include "ui/overlay.hpp"

namespace calib::ui {

std::vector<HitTarget> drawGuide(VkCommandBuffer command, vk::QuadPipeline overlay,
                                 vk::QuadPipeline ramp, VkExtent2D extent,
                                 patterns::OutputDepth depth, const guide::Session& session,
                                 std::span<const std::string_view> status, const Pointer& pointer,
                                 bool pointerVisible, float motion);

} // namespace calib::ui
