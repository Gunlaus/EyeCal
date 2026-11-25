#include "vk/renderer.hpp"
#include "patterns/ramp.hpp"
#include "ui/guide_view.hpp"
#include "vk/check.hpp"
#include "vk/surface_policy.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace calib::vk {
namespace {

template<class T, class Enumerate>
std::vector<T> list(Enumerate enumerate)
{
    for (;;) {
        std::uint32_t count = 0;
        check(enumerate(&count, static_cast<T*>(nullptr)), "Enumerate Vulkan objects");
        if (count == 0) {
            return {};
        }
        std::vector<T> items(count);
        const auto result = enumerate(&count, items.data());
        if (result == VK_INCOMPLETE) {
            continue;
        }
        check(result, "Read Vulkan objects");
        items.resize(count);
        return items;
    }
}

bool hasExtension(std::span<const VkExtensionProperties> extensions, const char* name)
{
    return name && std::ranges::any_of(extensions, [&](const auto& extension) {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}

std::optional<std::uint32_t> graphicsPresentQueue(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
    for (std::uint32_t index = 0; index < count; ++index) {
        if (families[index].queueCount == 0 || !(families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            continue;
        }
        VkBool32 present = VK_FALSE;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &present) == VK_SUCCESS &&
            present) {
            return index;
        }
    }
    return std::nullopt;
}

bool needsRebuild(VkResult result)
{
    return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT;
}

std::string_view formatName(VkFormat format)
{
    return format == VK_FORMAT_B8G8R8A8_UNORM ? "B8G8R8A8_UNORM" : "R8G8B8A8_UNORM";
}

std::string_view colorStatus(platform::OutputColor state)
{
    switch (state) {
    case platform::OutputColor::hdr:
        return "HDR is on. Turn it off for SDR tests.";
    case platform::OutputColor::sdr:
        return "Windows reports SDR. ACM status unknown.";
    case platform::OutputColor::unknown:
        return "HDR status unknown. Check display settings.";
    }
    return {};
}

} // namespace

void Renderer::initialize(SDL_Window* window)
{
    window_ = window;
    auto loader = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    if (!loader) {
        throw std::runtime_error("SDL did not provide a Vulkan loader.");
    }
    volkInitializeCustom(loader);
    if (volkGetInstanceVersion() < VK_API_VERSION_1_3) {
        throw std::runtime_error("EyeCal requires a Vulkan 1.3 loader and graphics driver.");
    }

    Uint32 count = 0;
    const auto* required = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!required) {
        throw std::runtime_error(std::string("Find SDL Vulkan extensions: ") + SDL_GetError());
    }
    std::vector<const char*> extensions(required, required + count);
    const auto instanceExtensions = list<VkExtensionProperties>([](auto* size, auto* items) {
        return vkEnumerateInstanceExtensionProperties(nullptr, size, items);
    });
    const auto* exclusive = platform::exclusiveDeviceExtension();
    const bool wantExclusive = exclusive &&
        hasExtension(instanceExtensions, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    if (wantExclusive) {
        extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    }
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "EyeCal";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &app;
    instanceInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    check(vkCreateInstance(&instanceInfo, nullptr, &instance_), "Create Vulkan 1.3 instance");
    volkLoadInstance(instance_);
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_)) {
        throw std::runtime_error(std::string("Create SDL Vulkan surface: ") + SDL_GetError());
    }

    std::uint32_t family = 0;
    const auto devices = list<VkPhysicalDevice>([&](auto* size, auto* items) {
        return vkEnumeratePhysicalDevices(instance_, size, items);
    });
    for (auto candidate : devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(candidate, &props);
        if (props.apiVersion < VK_API_VERSION_1_3) {
            continue;
        }

        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        VkPhysicalDeviceFeatures2 features{};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features.pNext = &features13;
        vkGetPhysicalDeviceFeatures2(candidate, &features);
        if (!features13.dynamicRendering) {
            continue;
        }
        const auto deviceExtensions = list<VkExtensionProperties>([&](auto* size, auto* items) {
            return vkEnumerateDeviceExtensionProperties(candidate, nullptr, size, items);
        });
        if (!hasExtension(deviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
            continue;
        }
        const auto queue = graphicsPresentQueue(candidate, surface_);
        if (!queue) {
            continue;
        }
        const auto formats = list<VkSurfaceFormatKHR>([&](auto* size, auto* items) {
            return vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface_, size, items);
        });
        if (!chooseSurfaceFormat(formats)) {
            continue;
        }
        family = *queue;
        physical_ = candidate;
        exclusiveEnabled_ = wantExclusive && hasExtension(deviceExtensions, exclusive);
        break;
    }
    if (!physical_) {
        throw std::runtime_error("No Vulkan 1.3 graphics device can present the required SDR UNORM format.");
    }

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = family;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    const std::array deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME, exclusive};
    VkPhysicalDeviceVulkan13Features features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features.dynamicRendering = VK_TRUE;
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &features;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = exclusiveEnabled_ ? 2 : 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    check(vkCreateDevice(physical_, &deviceInfo, nullptr, &device_), "Create Vulkan graphics device");
    volkLoadDevice(device_);
    vkGetDeviceQueue(device_, family, 0, &queue_);

    VkCommandPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool.queueFamilyIndex = family;
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device_, &pool, nullptr, &pool_), "Create command pool");
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = pool_;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(device_, &alloc, &command_), "Allocate command buffer");
    VkFenceCreateInfo fence{};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    check(vkCreateFence(device_, &fence, nullptr, &frameFence_), "Create frame fence");
    VkSemaphoreCreateInfo sem{};
    sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    check(vkCreateSemaphore(device_, &sem, nullptr, &acquired_), "Create acquire semaphore");
}

Renderer::~Renderer()
{
    if (device_) {
        dropSwapchain();
        vkDestroySemaphore(device_, acquired_, nullptr);
        vkDestroyFence(device_, frameFence_, nullptr);
        vkDestroyCommandPool(device_, pool_, nullptr);
        vkDestroyDevice(device_, nullptr);
    }
    if (surface_) {
        SDL_Vulkan_DestroySurface(instance_, surface_, nullptr);
    }
    if (instance_) {
        vkDestroyInstance(instance_, nullptr);
    }
}

void Renderer::dropSwapchain()
{
    if (swapchain_) {
        vkDeviceWaitIdle(device_);
        destroyQuadPipeline(device_, ramp_);
        ramp_ = {};
        destroyQuadPipeline(device_, overlay_);
        overlay_ = {};
        for (auto semaphore : presented_) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }
        for (auto view : views_) {
            vkDestroyImageView(device_, view, nullptr);
        }
        if (exclusiveAcquired_) {
            platform::releaseExclusive(device_, swapchain_);
        }
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }
    swapchain_ = VK_NULL_HANDLE;
    views_.clear();
    images_.clear();
    presented_.clear();
    exclusiveAcquired_ = false;
    rebuild_ = true;
}

bool Renderer::buildSwapchain(bool exclusiveRequested)
{
    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(window_, &width, &height) || width <= 0 || height <= 0) {
        return false;
    }
    const VkExtent2D pixels{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    VkSurfaceCapabilitiesKHR caps{};
    const auto queried = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_, surface_, &caps);
    if (needsRebuild(queried)) {
        return false;
    }
    check(queried, "Query surface capabilities");
    const auto extent = chooseNativeExtent(caps, pixels);
    if (!extent) {
        return false;
    }
    if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) ||
        !(caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)) {
        throw std::runtime_error("The surface cannot render direct, unrotated color attachments.");
    }
    const auto formats = list<VkSurfaceFormatKHR>([&](auto* size, auto* items) {
        return vkGetPhysicalDeviceSurfaceFormatsKHR(physical_, surface_, size, items);
    });
    const auto format = chooseSurfaceFormat(formats);
    if (!format) {
        throw std::runtime_error("The current surface has no supported SDR UNORM format.");
    }

    constexpr std::array alphaModes{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};
    auto alpha = std::ranges::find_if(alphaModes, [&](auto mode) {
        return (caps.supportedCompositeAlpha & mode) != 0;
    });
    if (alpha == alphaModes.end()) {
        throw std::runtime_error("The surface has no supported composite alpha mode.");
    }

    dropSwapchain();
    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface_;
    info.minImageCount = chooseImageCount(caps);
    info.imageFormat = format->format;
    info.imageColorSpace = format->colorSpace;
    info.imageExtent = *extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha = *alpha;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;
    const auto created = platform::createSwapchain(window_, physical_, device_, info,
                                                   exclusiveRequested, exclusiveEnabled_, swapchain_);
    if (needsRebuild(created.result)) {
        return false;
    }
    check(created.result, "Create UNORM swapchain");
    exclusiveAcquired_ = created.exclusive;
    extent_ = *extent;
    format_ = format->format;
    images_ = list<VkImage>([&](auto* size, auto* items) {
        return vkGetSwapchainImagesKHR(device_, swapchain_, size, items);
    });
    views_.resize(images_.size());
    presented_.resize(images_.size());
    for (std::size_t index = 0; index < images_.size(); ++index) {
        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = images_[index];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format_;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        check(vkCreateImageView(device_, &view, nullptr, &views_[index]), "Create swapchain image view");
        VkSemaphoreCreateInfo sem{};
        sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        check(vkCreateSemaphore(device_, &sem, nullptr, &presented_[index]), "Create present semaphore");
    }
    overlay_ = ui::createOverlay(device_, format_);
    ramp_ = patterns::createRamp(device_, format_);
    rebuild_ = false;
    return true;
}

std::vector<ui::HitTarget> Renderer::record(std::uint32_t index, const Frame& frame)
{
    check(vkResetCommandBuffer(command_, 0), "Reset command buffer");
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(command_, &begin), "Begin drawing");

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = images_[index];
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(command_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);
    VkRenderingAttachmentInfo target{};
    target.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    target.imageView = views_[index];
    target.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    target.clearValue.color = {{64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f, 1.0f}};
    VkRenderingInfo render{};
    render.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    render.renderArea = {{0, 0}, extent_};
    render.layerCount = 1;
    render.colorAttachmentCount = 1;
    render.pColorAttachments = &target;
    vkCmdBeginRendering(command_, &render);

    std::array<std::string_view, 5> lines{};
    std::size_t lineCount = 0;
    std::string surface;
    if (frame.diagnostics) {
        surface = std::string(formatName(format_)) + "   " + std::to_string(extent_.width) +
                  " x " + std::to_string(extent_.height) + "   FIFO";
        lines[lineCount++] = surface;
        lines[lineCount++] = exclusiveAcquired_ ? "Fullscreen: Vulkan exclusive"
            : frame.exclusiveRequested ? "Fullscreen: mode set, Vulkan not exclusive"
                                       : "Fullscreen: borderless";
    }
    if (frame.diagnostics || frame.outputColor == platform::OutputColor::hdr) {
        lines[lineCount++] = colorStatus(frame.outputColor);
    }
    if (!frame.notice.empty()) {
        lines[lineCount++] = frame.notice;
    }
    std::vector<ui::HitTarget> targets;
    if (frame.licensePage.empty()) {
        targets = ui::drawGuide(command_, overlay_, ramp_, extent_, patterns::OutputDepth::unorm8,
                                frame.guide, std::span{lines}.first(lineCount), frame.pointer,
                                frame.pointerVisible);
    } else {
        std::vector<std::string_view> page(frame.licensePage.begin(), frame.licensePage.end());
        if (frame.outputColor == platform::OutputColor::hdr) {
            page.insert(page.begin(), colorStatus(frame.outputColor));
        }
        ui::drawOverlay(command_, overlay_, extent_, page);
    }
    vkCmdEndRendering(command_);

    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    vkCmdPipelineBarrier(command_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);
    check(vkEndCommandBuffer(command_), "Finish drawing");
    return targets;
}

bool Renderer::draw(const Frame& frame, std::vector<ui::HitTarget>& targets)
{
    targets.clear();
    if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED) {
        return false;
    }
    if (rebuild_ && !buildSwapchain(frame.exclusiveRequested && frame.focused)) {
        return false;
    }
    check(vkWaitForFences(device_, 1, &frameFence_, VK_TRUE, UINT64_MAX), "Wait for previous frame");
    std::uint32_t index = 0;
    const auto acquired = vkAcquireNextImageKHR(device_, swapchain_, 100'000'000, acquired_,
                                                 VK_NULL_HANDLE, &index);
    if (needsRebuild(acquired)) {
        invalidate();
        return false;
    }
    if (acquired == VK_TIMEOUT || acquired == VK_NOT_READY) {
        return false;
    }
    if (acquired != VK_SUBOPTIMAL_KHR) {
        check(acquired, "Acquire swapchain image");
    }
    auto nextTargets = record(index, frame);
    check(vkResetFences(device_, 1, &frameFence_), "Reset frame fence");
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &acquired_;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &presented_[index];
    check(vkQueueSubmit(queue_, 1, &submit, frameFence_), "Submit frame");

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &presented_[index];
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &index;
    const auto result = vkQueuePresentKHR(queue_, &present);
    if (needsRebuild(result)) {
        invalidate();
        return false;
    }
    if (result != VK_SUBOPTIMAL_KHR) {
        check(result, "Present frame");
    }
    targets = std::move(nextTargets);
    // A working SUBOPTIMAL presentation may persist until the next display event.
    return true;
}

} // namespace calib::vk
