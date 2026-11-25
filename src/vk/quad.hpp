#pragma once

#include <volk.h>
#include <cstdint>
#include <span>

namespace calib::vk {

struct QuadPipeline {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

inline constexpr auto quadStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

QuadPipeline createQuadPipeline(VkDevice device, VkFormat format,
                                std::span<const std::uint32_t> fragmentWords,
                                std::uint32_t pushBytes, bool blend, bool hdrUi = false);
void destroyQuadPipeline(VkDevice device, QuadPipeline quad);
void bindQuadPipeline(VkCommandBuffer command, QuadPipeline quad, VkExtent2D extent,
                      VkRect2D scissor);

} // namespace calib::vk
