#include "ui/overlay.hpp"
#include "ui/sdf_font.hpp"
#include "vk/check.hpp"

#include "overlay_frag.hpp"
#include "overlay_vert.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace calib::ui {
namespace {

struct Push {
    std::array<int, 4> rect{};
    std::array<int, 2> viewport{};
    int cellScale = 1;
    int solid = 0;
    std::array<std::uint32_t, 16> glyph{};
    std::array<float, 4> ink{};
};
static_assert(offsetof(Push, glyph) == 32);
static_assert(offsetof(Push, ink) == 96);
static_assert(sizeof(Push) == 112);
constexpr auto stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

VkShaderModule shaderModule(VkDevice device, std::span<const std::uint32_t> words)
{
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = words.size_bytes();
    info.pCode = words.data();
    VkShaderModule result = VK_NULL_HANDLE;
    vk::check(vkCreateShaderModule(device, &info, nullptr, &result), "Create overlay shader");
    return result;
}

} // namespace

OverlayPipelines createOverlay(VkDevice device, VkFormat format)
{
    OverlayPipelines overlay{};
    VkShaderModule vertex = VK_NULL_HANDLE;
    VkShaderModule fragment = VK_NULL_HANDLE;
    try {
        vertex = shaderModule(device, overlay_vert);
        fragment = shaderModule(device, overlay_frag);
        const VkPushConstantRange range{stages, 0, sizeof(Push)};
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &range;
        vk::check(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &overlay.layout),
                  "Create overlay layout");

        const std::array shaderStages{
            VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                           nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex, "main", nullptr},
            VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                           nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment, "main", nullptr},
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
        target.blendEnable = VK_TRUE;
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
        pipeline.layout = overlay.layout;
        vk::check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline, nullptr,
                                           &overlay.pipeline),
                  "Create overlay pipeline");
    } catch (...) {
        destroyOverlay(device, overlay);
        vkDestroyShaderModule(device, fragment, nullptr);
        vkDestroyShaderModule(device, vertex, nullptr);
        throw;
    }
    vkDestroyShaderModule(device, fragment, nullptr);
    vkDestroyShaderModule(device, vertex, nullptr);
    return overlay;
}

void destroyOverlay(VkDevice device, OverlayPipelines overlay)
{
    vkDestroyPipeline(device, overlay.pipeline, nullptr);
    vkDestroyPipelineLayout(device, overlay.layout, nullptr);
}

void drawOverlay(VkCommandBuffer command, OverlayPipelines overlay, VkExtent2D extent,
                 std::span<const std::string_view> lines)
{
    if (lines.empty()) {
        return;
    }
    const VkViewport viewport{0, 0, static_cast<float>(extent.width),
                             static_cast<float>(extent.height), 0, 1};
    const VkRect2D scissor{{0, 0}, extent};
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, overlay.pipeline);
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdSetScissor(command, 0, 1, &scissor);
    Push push{};
    push.viewport = {static_cast<int>(extent.width), static_cast<int>(extent.height)};
    std::size_t longest = 0;
    for (auto line : lines) {
        longest = std::max(longest, line.size());
    }
    const int baseWidth = 24 + glyphWidth * static_cast<int>(longest);
    const int baseHeight = 24 + 25 * static_cast<int>(lines.size());
    push.cellScale = std::max(1, std::min({push.viewport[0] / 960,
                                         (push.viewport[0] - 24) / baseWidth,
                                         (push.viewport[1] - 24) / baseHeight}));
    const int scale = push.cellScale;
    const int margin = 12 * scale;
    auto draw = [&] {
        vkCmdPushConstants(command, overlay.layout, stages, 0, sizeof(push), &push);
        vkCmdDraw(command, 6, 1, 0, 0);
    };

    push.rect = {margin, margin, baseWidth * scale, baseHeight * scale};
    push.solid = 1;
    push.ink = {0.025f, 0.025f, 0.025f, 1.0f};
    draw();

    push.solid = 0;
    push.ink = {0.96f, 0.96f, 0.96f, 1.0f};
    int y = margin * 2;
    for (auto line : lines) {
        int x = margin * 2;
        for (unsigned char c : line) {
            if (c != ' ') {
                const auto glyph = c >= 32 && c <= 126 ? c : '?';
                push.glyph = glyphs[glyph - 32];
                push.rect = {x, y, glyphWidth * scale, glyphHeight * scale};
                draw();
            }
            x += glyphWidth * scale;
        }
        y += 25 * scale;
    }
}

} // namespace calib::ui
