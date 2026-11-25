# Verified platform behavior

## Windows composition and fullscreen

**VERIFIED from Microsoft documentation.** A DXGI flip-model swapchain can use DirectFlip or Independent Flip when the window, buffer and output conditions allow it. DWM can resume composition when other desktop content appears. Microsoft also documents that Windows Advanced Color and automatic color management can affect the desktop color pipeline. These DirectX presentation paths are documented separately from Vulkan Win32 WSI. [Microsoft DXGI flip model guidance](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model) · [Microsoft Advanced Color documentation](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)

**VERIFIED from the Vulkan specification.** The Windows fullscreen-exclusive extension separates requesting an application-controlled fullscreen swapchain from acquiring exclusive access. An acquisition may fail and presentation may later lose exclusivity. M1 queries the extension and surface support, observes the acquisition result and falls back when needed. [Khronos VK_EXT_full_screen_exclusive](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_full_screen_exclusive.html) · [Khronos acquisition semantics](https://docs.vulkan.org/refpages/latest/refpages/source/vkAcquireFullScreenExclusiveModeEXT.html)

**UNVERIFIED on Vulkan Win32.** The actual presentation path, whether OS or vendor software applies a transform, and GPU-driver or panel dithering are specific to the output chain. The cited documents give no universal bit-preservation or dithering guarantee in borderless or exclusive mode. M1 leaves the driver and OS color settings alone. The overlay reports the Vulkan image format, pixel extent, present mode and whether the Vulkan exclusive request succeeded. Check both modes on hardware. [Microsoft Advanced Color architecture](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range) · [Khronos swapchain color space and image format](https://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainCreateInfoKHR.html)

## Automatic colour management and 10-bit comparisons

**VERIFIED from Microsoft documentation.** When Windows automatic colour management (ACM) is active, DWM converts app content to its composition colour space before blending, using a matrix and 1D LUT. The display kernel handles the output conversion. Microsoft describes the SDR Advanced Color pipeline as largely the same as the HDR pipeline, with display-referred luminance for SDR. `IDXGIOutput6::GetDesc1` reports plain SDR and SDR with ACM as the same `DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709` value. [Microsoft Advanced Color documentation](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)

**UNVERIFIED in EyeCal.** The app’s DXGI query cannot detect whether ACM is active on SDR. App-side detection of ACM is unverified. The English intro asks users to turn off HDR and auto colour management in Windows display settings before the SDR tests. [Microsoft Advanced Color documentation](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)

**VERIFIED historical limit.** Microsoft states that before Advanced Color, DWM restricted windowed app output to eight bits per colour channel. With Advanced Color active, Microsoft documents FP16 composition. [Microsoft Advanced Color documentation](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)

**UNVERIFIED for Vulkan borderless 10-bit presentation.** Whether the historical windowed limit affects a Vulkan 10-bit borderless swapchain is unverified. An advertised swapchain format does not establish output bit depth or an independent presentation path. The future screen 6c will report “Inconclusive in borderless mode” if either measured pass ran without granted Vulkan exclusive presentation. A 10-bit run can lose precision in windowed composition and other downstream stages. [Microsoft Advanced Color documentation](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range) · [Screen 6c decision rules](ARCHITECTURE.md#screen-6c-bit-depth-probe-design)

## Windows HDR detection

**VERIFIED from Microsoft documentation.** `IDXGIOutput6::GetDesc1` exposes the current output color space in `DXGI_OUTPUT_DESC1::ColorSpace`. Microsoft documents `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` for HDR and `DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709` for SDR. The latter can also indicate SDR with automatic color management. DXGI reports the current state and cannot describe all inactive modes. The app uses DXGI for this display query while rendering with Vulkan. [Microsoft Advanced Color detection](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range#idxgioutput6) · [Microsoft IDXGIOutput6::GetDesc1](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_6/nf-dxgi1_6-idxgioutput6-getdesc1)

**Needs on-device check.** M1 matches the Windows window monitor to the enumerated DXGI output, treats unavailable or unfamiliar codes as unknown, and refreshes the query after display changes. Confirm the reported state on Windows 10 and 11, including mixed HDR and SDR displays and mirrored outputs. [Microsoft MonitorFromWindow](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfromwindow) · [Microsoft DXGI output descriptor](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_output_desc)

## Fullscreen size and presentation

**VERIFIED from SDL and Vulkan documentation.** SDL uses a null fullscreen mode to request borderless desktop fullscreen. The actual framebuffer size in pixels is queried separately from window coordinates. Vulkan requires a supported format and color space pair and exposes surface extent constraints. FIFO is a required present mode. [SDL fullscreen display mode](https://wiki.libsdl.org/SDL3/SDL_SetWindowFullscreenMode) · [SDL pixel size](https://wiki.libsdl.org/SDL3/SDL_GetWindowSizeInPixels) · [Khronos surface formats](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPhysicalDeviceSurfaceFormatsKHR.html) · [Khronos present modes](https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html)

**UNVERIFIED on device.** Matching the swapchain to the SDL framebuffer establishes 1:1 rendering inside the app. Panel-native scanout, GPU scaling, overscan and compositor behavior require hardware checks. M1 follows the selected desktop mode and leaves the display's hardware settings to the user.

For swapchain teardown on the baseline Vulkan path, M1 uses device idle, the conventional fallback described by Khronos. The core API has limited presentation-completion guarantees. This is a reason to stress-test toggling and exit on device. [Khronos swapchain semaphore reuse and teardown](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html#_problem_3_shutdown)


## Pointer input on fullscreen displays

**VERIFIED from SDL documentation.** SDL mouse motion and button positions are relative to the window. The window size in window coordinates and its size in pixels have separate queries. EyeCal converts positions by the ratio on each axis before testing Vulkan UI rectangles. SDL exposes whole-tick wheel deltas, reports a flipped scroll direction, and exposes cursor visibility through `SDL_ShowCursor` and `SDL_HideCursor`. [SDL mouse motion](https://wiki.libsdl.org/SDL3/SDL_MouseMotionEvent) · [SDL mouse buttons](https://wiki.libsdl.org/SDL3/SDL_MouseButtonEvent) · [SDL window pixel size](https://wiki.libsdl.org/SDL3/SDL_GetWindowSizeInPixels) · [SDL mouse wheel](https://wiki.libsdl.org/SDL3/SDL_MouseWheelEvent) · [SDL cursor visibility](https://wiki.libsdl.org/SDL3/SDL_HideCursor)

**UNVERIFIED on device.** Pointer alignment across mixed-DPI monitors, cursor behavior during Windows fullscreen mode switches, and touchpad scroll behavior require hardware testing. [On-device checklist](ON_DEVICE_CHECKLIST.md#mouse-and-keyboard-controls)

## UI brightness on the physical display

**UNVERIFIED on device.** EyeCal's dark UI lowers its own encoded background values. Whether a given OLED, projector or TV changes brightness during screen transitions depends on the display and settings. Check for visible shifts on a real display. [On-device checklist](ON_DEVICE_CHECKLIST.md#visual-chrome)

**VERIFIED signal conventions.** Microsoft defines full-range 8-bit code values as 0–255 and conventional limited range as 16–235. EyeCal draws exact RGB code values into its 8-bit UNORM attachment. Actual device range, panel scaling and visibility of individual steps remain unverified on device. [Microsoft nominal-range definitions](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/ne-mfobjects-mfnominalrange)

## 10-bit probe

**VERIFIED API semantics.** EyeCal asks Vulkan for supported surface format and colour-space pairs and accepts only the documented 8-bit UNORM formats or an advertised packed 10-bit UNORM format paired with the SDR colour space. The 10-bit format describes the swapchain attachment. [Vulkan surface formats](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPhysicalDeviceSurfaceFormatsKHR.html) · [Vulkan swapchain create info](https://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainCreateInfoKHR.html)

**UNVERIFIED on device.** Effective 10-bit precision in borderless Vulkan, propagation through a specific exclusive presentation path, hardware stability under repeated format changes, and interpretation of moving grain or flicker require hardware checks. The probe labels a nonexclusive comparison inconclusive. Exclusive mode permits a qualified likely cause when the comparable 10-bit pass shows visible banding with no improvement. [Microsoft Advanced Color and historical DWM limit](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)
