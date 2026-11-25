# EyeCal

**Calibrate your display by eye, one setting at a time.**

Can you make out the darkest step? Do the brightest whites still look distinct? EyeCal is being built to help you answer those questions on an SDR monitor, TV, or projector. Each guided screen will pair a fullscreen test pattern with plain-language instructions and an example of what a bad result looks like.

EyeCal is designed for people without a colorimeter. Measuring color accuracy still requires a meter.

> **Project status · Milestone 1**  
> The Windows prototype, `CalibGuide.exe`, opens a fullscreen grey field and shows how the app is presenting it. The guided calibration screens are planned for later milestones.

## Try the Windows prototype

You’ll need Windows 10 or 11 and a Vulkan 1.3 graphics driver. Set Windows to the display’s recommended resolution, then build the executable below or download `CalibGuide-Windows-x64` from a successful Windows run in [GitHub Actions](https://github.com/Gunlaus/EyeCal/actions/workflows/build.yml). Unzip the artifact and double-click `CalibGuide.exe`.

| Key | What it does |
| --- | --- |
| **F1** | Show or hide the swapchain format, pixel dimensions, present mode, and HDR status |
| **F11** | Switch fullscreen modes. F1 shows whether Vulkan granted exclusive presentation |
| **F2** | Read the bundled licenses. Use **Page Up** and **Page Down** to move between pages |
| **Esc** | Exit |

## What’s planned

- **Set black and white levels.** Find crushed shadows, clipped highlights, and full-range versus limited-range mismatches.
- **Tune gamma and sharpness.** Compare small checkerboards with solid greys, then use fine lines to spot blur, halos, and overscan.
- **Investigate banding.** Compare ramps with and without dithering, switch between 8-bit and 10-bit paths where supported, and check whether banding is concentrated in the shadows.
- **Check uniformity.** Use flat fields to look for uneven brightness and bad pixels, then save a plain-text summary.

## Build from source

Install Visual Studio with **Desktop development with C++**, CMake 3.24 or newer, and the Vulkan SDK with `glslc`. CMake fetches pinned build dependencies.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The executable is at `build/Release/CalibGuide.exe`. SDL, volk, shaders, and the font atlas are built in. The app uses the system graphics driver and makes no network requests.

<details>
<summary>Optional Linux build and CPU-only checks</summary>

Install a C++20 compiler, CMake, `glslc`, and SDL’s X11 development dependencies. Linux supports borderless fullscreen and reports HDR status as unknown.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSDL_WAYLAND=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For CPU checks without SDL or a shader compiler, add `-DCALIBGUIDE_BUILD_APP=OFF` when configuring.

</details>

[Architecture](docs/ARCHITECTURE.md) · [Pattern definitions](docs/PATTERNS.md) · [Verified platform behavior](docs/VERIFIED.md) · [On-device checklist](docs/ON_DEVICE_CHECKLIST.md) · [Third-party notices](THIRD_PARTY.md)
