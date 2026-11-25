#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "vk/surface_policy.hpp"

#include <array>

using namespace calib::vk;

TEST_CASE("Surface selection prefers BGRA SDR and falls back to RGBA SDR")
{
    constexpr auto sdr = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    constexpr auto hdr = VK_COLOR_SPACE_HDR10_ST2084_EXT;
    constexpr std::array formats{
        VkSurfaceFormatKHR{VK_FORMAT_R8G8B8A8_UNORM, sdr},
        VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, hdr},
        VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, sdr},
    };
    const auto preferred = chooseSurfaceFormat(formats);
    REQUIRE(preferred.has_value());
    CHECK(preferred->format == VK_FORMAT_B8G8R8A8_UNORM);
    CHECK(preferred->colorSpace == sdr);

    const auto fallback = chooseSurfaceFormat(std::span{formats}.first(2));
    REQUIRE(fallback.has_value());
    CHECK(fallback->format == VK_FORMAT_R8G8B8A8_UNORM);

    constexpr std::array unsupported{VkSurfaceFormatKHR{VK_FORMAT_R16G16B16A16_SFLOAT, sdr}};
    CHECK_FALSE(chooseSurfaceFormat(unsupported).has_value());
    CHECK_FALSE(chooseSurfaceFormat({}).has_value());

    constexpr std::array anyFormat{VkSurfaceFormatKHR{VK_FORMAT_UNDEFINED, sdr}};
    CHECK_FALSE(chooseSurfaceFormat(anyFormat).has_value());
}

TEST_CASE("Swapchain extent must match the nonzero SDL pixel framebuffer")
{
    VkSurfaceCapabilitiesKHR caps{};
    caps.minImageExtent = {64, 64};
    caps.maxImageExtent = {3840, 2160};
    caps.currentExtent = {1920, 1080};
    CHECK(chooseNativeExtent(caps, {1920, 1080}).has_value());
    CHECK_FALSE(chooseNativeExtent(caps, {1280, 720}).has_value());
    CHECK_FALSE(chooseNativeExtent(caps, {0, 0}).has_value());

    const auto unspecified = std::numeric_limits<std::uint32_t>::max();
    caps.currentExtent = {unspecified, unspecified};
    CHECK(chooseNativeExtent(caps, {2560, 1440}).has_value());
    CHECK_FALSE(chooseNativeExtent(caps, {3841, 2160}).has_value());
}

TEST_CASE("Swapchain buffering respects both bounded and unbounded surface limits")
{
    VkSurfaceCapabilitiesKHR caps{};
    caps.minImageCount = 2;
    CHECK(chooseImageCount(caps) == 3);
    caps.maxImageCount = 2;
    CHECK(chooseImageCount(caps) == 2);
    caps.minImageCount = 4;
    caps.maxImageCount = 0;
    CHECK(chooseImageCount(caps) == 4);
}
