#include "vk/quad.hpp"
#include "vk/check.hpp"
#include "quad_vert.hpp"

#include <array>

namespace calib::vk {
namespace {

VkShaderModule shaderModule(VkDevice device, std::span<const std::uint32_t> words)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = words.size_bytes();
    info.pCode = words.data();
    VkShaderModule result = VK_NULL_HANDLE;
    check(vkCreateShaderModule(device, &info, nullptr, &result), "Create quad shader");
    return result;
}

} // namespace

QuadPipeline createQuadPipeline(VkDevice device, VkFormat format,
                                std::span<const std::uint32_t> fragmentWords,
                                std::uint32_t pushBytes, bool blendEnabled, bool hdrUi)
{
    QuadPipeline quad{};
    VkShaderModule vertex = VK_NULL_HANDLE;
    VkShaderModule fragment = VK_NULL_HANDLE;
    try {
        vertex = shaderModule(device, quad_vert);
        fragment = shaderModule(device, fragmentWords);
        const VkPushConstantRange range{quadStages, 0, pushBytes};
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &range;
        check(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &quad.layout),
                  "Create quad layout");

        const VkSpecializationMapEntry hdrEntry{0, 0, sizeof(std::uint32_t)};
        const std::uint32_t hdrValue = hdrUi ? 1u : 0u;
        const VkSpecializationInfo hdrSpecialization{1, &hdrEntry, sizeof(hdrValue), &hdrValue};
        const std::array shaderStages{
            VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                           nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex, "main", nullptr},
            VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                           nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment, "main", blendEnabled ? &hdrSpecialization : nullptr},
        };
        VkPipelineVertexInputStateCreateInfo inputs{};
        inputs.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VkPipelineInputAssemblyStateCreateInfo assembly{};
        assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{};
        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1.0f;
        VkPipelineMultisampleStateCreateInfo samples{};
        samples.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samples.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState target{};
        target.blendEnable = blendEnabled ? VK_TRUE : VK_FALSE;
        target.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        target.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        target.colorBlendOp = VK_BLEND_OP_ADD;
        target.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        target.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        target.alphaBlendOp = VK_BLEND_OP_ADD;
        target.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend{};
        blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.attachmentCount = 1;
        blend.pAttachments = &target;
        constexpr std::array states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{};
        dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic.dynamicStateCount = static_cast<std::uint32_t>(states.size());
        dynamic.pDynamicStates = states.data();
        VkPipelineRenderingCreateInfo rendering{};
        rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &format;
        VkGraphicsPipelineCreateInfo pipeline{};
        pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline.pNext = &rendering;
        pipeline.stageCount = static_cast<std::uint32_t>(shaderStages.size());
        pipeline.pStages = shaderStages.data();
        pipeline.pVertexInputState = &inputs;
        pipeline.pInputAssemblyState = &assembly;
        pipeline.pViewportState = &viewport;
        pipeline.pRasterizationState = &raster;
        pipeline.pMultisampleState = &samples;
        pipeline.pColorBlendState = &blend;
        pipeline.pDynamicState = &dynamic;
        pipeline.layout = quad.layout;
        check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline, nullptr,
                                           &quad.pipeline),
                  "Create quad pipeline");
    } catch (...) {
        destroyQuadPipeline(device, quad);
        vkDestroyShaderModule(device, fragment, nullptr);
        vkDestroyShaderModule(device, vertex, nullptr);
        throw;
    }
    vkDestroyShaderModule(device, fragment, nullptr);
    vkDestroyShaderModule(device, vertex, nullptr);
    return quad;
}

void destroyQuadPipeline(VkDevice device, QuadPipeline quad)
{
    vkDestroyPipeline(device, quad.pipeline, nullptr);
    vkDestroyPipelineLayout(device, quad.layout, nullptr);
}

void bindQuadPipeline(VkCommandBuffer command, QuadPipeline quad, VkExtent2D extent,
                      VkRect2D scissor)
{
    const VkViewport viewport{0, 0, static_cast<float>(extent.width),
                             static_cast<float>(extent.height), 0, 1};
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.pipeline);
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);
}

} // namespace calib::vk
