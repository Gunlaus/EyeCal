# EyeCal

**Adjust your display while the test pattern is still on screen.**

EyeCal is a Windows-first display test suite for checking SDR levels, sharpness, banding, colour, and HDR10 by eye. Each test tells you which display, GPU, or playback setting to open and what to check after changing it.

## Start a test

Get **EyeCal-Windows-x64** from a successful [GitHub Actions run](https://github.com/Gunlaus/EyeCal/actions/workflows/build.yml). Unzip it and run `EyeCal.exe` on Windows 10 or 11 with a Vulkan 1.3 driver. Set Windows to the display’s recommended resolution.

- **Start SDR** for RGB range, black level, white clipping, gamma, sharpness, and banding. Turn off Windows HDR and auto colour management for these checks.
- **Colour tests** for greyscale tint, saturation, individual channel clipping, and solid fields.
- **HDR tests** for peak clipping, near-black detail, reference white, and brightness at three window sizes. Turn on Windows **Use HDR**. These tests require Vulkan to expose a compatible 10-bit HDR10 swapchain. The app shows why a test is paused if that path is unavailable.
- **All tests** jumps directly to any pattern. Keep the display’s picture menu open while making adjustments. The top-right panel shows the app’s active signal format and fullscreen state.

| Control | Action |
| --- | --- |
| **Left / Right**, **Enter**, mouse wheel | Previous or next test |
| **Esc** | Open or close the test library |
| **Tab**, **Shift+Tab**, **Enter** | Focus and activate a control |
| **D** | Toggle dithering on the first ramp; change the algorithm on the split ramp |
| **B / M** | Switch probe bit depth or toggle the moving shadow ramp |
| **F1** | Expand or collapse signal details |
| **F2 / F3 / F11** | Licences, text size, exclusive or borderless fullscreen |

EyeCal can help isolate clipping, unevenness, and likely causes of banding. A meter is needed to measure colour accuracy, luminance, or actual panel bit depth. The HDR luminance labels are encoded signal targets.

## Build

Install Visual Studio with **Desktop development with C++**, CMake 3.24 or newer, and the Vulkan SDK with `glslc`.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The standalone Windows executable is at `build/Release/EyeCal.exe`. Build dependencies are pinned. The app runs locally and makes no network requests.

<details>
<summary>Linux and CPU-only checks</summary>

Linux builds with a C++20 compiler, CMake, `glslc`, and SDL’s X11 development dependencies. The portable platform layer supports SDR and reports Windows HDR detection as unknown.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSDL_WAYLAND=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Add `-DEYECAL_BUILD_APP=OFF` when configuring to run the CPU checks without SDL or `glslc`.

</details>

[Pattern definitions](docs/PATTERNS.md) · [Architecture](docs/ARCHITECTURE.md) · [Platform findings](docs/VERIFIED.md) · [On-device checklist](docs/ON_DEVICE_CHECKLIST.md) · [Third-party notices](THIRD_PARTY.md)
