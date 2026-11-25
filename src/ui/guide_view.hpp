#pragma once

#include "guide/guide.hpp"
#include "ui/overlay.hpp"

namespace calib::ui {

void drawGuide(VkCommandBuffer command, vk::QuadPipeline overlay, vk::QuadPipeline ramp,
               VkExtent2D extent, patterns::OutputDepth depth, const guide::Session& session,
               std::span<const std::string_view> status);

} // namespace calib::ui
