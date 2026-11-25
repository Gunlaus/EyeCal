#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "platform/platform.hpp"
#include "vk/surface_policy.hpp"
#include <vulkan/vulkan_win32.h>
#include <SDL3/SDL_video.h>

namespace calib::platform {
namespace {

HMONITOR monitorFor(SDL_Window* window)
{
    const auto props = SDL_GetWindowProperties(window);
    const auto handle = static_cast<HWND>(
        SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    return handle ? MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST) : nullptr;
}

} // namespace

OutputColor outputColor(SDL_Window* window)
{
    using Microsoft::WRL::ComPtr;
    const auto monitor = monitorFor(window);
    ComPtr<IDXGIFactory1> factory;
    if (!monitor || FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())))) {
        return OutputColor::unknown;
    }
    for (UINT adapterIndex = 0;; ++adapterIndex) {
        ComPtr<IDXGIAdapter1> adapter;
        if (FAILED(factory->EnumAdapters1(adapterIndex, adapter.GetAddressOf()))) {
            return OutputColor::unknown;
        }
        for (UINT outputIndex = 0;; ++outputIndex) {
            ComPtr<IDXGIOutput> output;
            const auto status = adapter->EnumOutputs(outputIndex, output.GetAddressOf());
            if (status == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(status)) {
                return OutputColor::unknown;
            }
            DXGI_OUTPUT_DESC desc{};
            if (FAILED(output->GetDesc(&desc)) || !desc.AttachedToDesktop || desc.Monitor != monitor) {
                continue;
            }
            ComPtr<IDXGIOutput6> output6;
            DXGI_OUTPUT_DESC1 details{};
            if (FAILED(output.As(&output6)) || FAILED(output6->GetDesc1(&details))) {
                return OutputColor::unknown;
            }
            switch (details.ColorSpace) {
            case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
                return OutputColor::hdr;
            case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
                return OutputColor::sdr;
            default:
                return OutputColor::unknown;
            }
        }
    }
}

const char* exclusiveDeviceExtension()
{
    return VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME;
}

SwapchainCreation createSwapchain(SDL_Window* window, VkPhysicalDevice physical,
                                  VkDevice device, VkSwapchainCreateInfoKHR info,
                                  bool exclusiveRequested, bool exclusiveEnabled,
                                  VkSwapchainKHR& swapchain)
{
    const auto acquire = exclusiveEnabled
        ? reinterpret_cast<PFN_vkAcquireFullScreenExclusiveModeEXT>(
              vkGetDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT"))
        : nullptr;
    if (exclusiveRequested && acquire && vkGetPhysicalDeviceSurfaceCapabilities2KHR) {
        VkSurfaceFullScreenExclusiveWin32InfoEXT target{};
        target.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
        target.hmonitor = monitorFor(window);
        VkSurfaceFullScreenExclusiveInfoEXT request{};
        request.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
        request.pNext = &target;
        request.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;
        VkPhysicalDeviceSurfaceInfo2KHR surface{};
        surface.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
        surface.pNext = &request;
        surface.surface = info.surface;
        VkSurfaceCapabilitiesFullScreenExclusiveEXT support{};
        support.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_FULL_SCREEN_EXCLUSIVE_EXT;
        VkSurfaceCapabilities2KHR caps{};
        caps.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
        caps.pNext = &support;
        if (target.hmonitor && vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical, &surface, &caps) ==
                                   VK_SUCCESS && support.fullScreenExclusiveSupported &&
            vk::chooseNativeExtent(caps.surfaceCapabilities, info.imageExtent) &&
            info.minImageCount >= caps.surfaceCapabilities.minImageCount &&
            (caps.surfaceCapabilities.maxImageCount == 0 ||
             info.minImageCount <= caps.surfaceCapabilities.maxImageCount) &&
            (caps.surfaceCapabilities.supportedUsageFlags & info.imageUsage) == info.imageUsage &&
            (caps.surfaceCapabilities.supportedTransforms & info.preTransform) &&
            (caps.surfaceCapabilities.supportedCompositeAlpha & info.compositeAlpha)) {
            info.pNext = &request;
            const auto result = vkCreateSwapchainKHR(device, &info, nullptr, &swapchain);
            if (result == VK_SUCCESS) {
                if (acquire(device, swapchain) == VK_SUCCESS) {
                    return {VK_SUCCESS, true};
                }
                vkDestroySwapchainKHR(device, swapchain, nullptr);
                swapchain = VK_NULL_HANDLE;
            }
        }
    }
    VkSurfaceFullScreenExclusiveInfoEXT disallow{};
    if (exclusiveEnabled) {
        disallow.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
        disallow.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
        info.pNext = &disallow;
    }
    return {vkCreateSwapchainKHR(device, &info, nullptr, &swapchain), false};
}

void releaseExclusive(VkDevice device, VkSwapchainKHR swapchain)
{
    const auto release = reinterpret_cast<PFN_vkReleaseFullScreenExclusiveModeEXT>(
        vkGetDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT"));
    if (release) {
        release(device, swapchain);
    }
}

} // namespace calib::platform
