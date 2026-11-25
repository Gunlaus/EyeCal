# EyeCal

**Calibrate your display by eye, one setting at a time.**

Can you make out the darkest step? Do the brightest whites still look distinct? EyeCal is being built to help you answer those questions on an SDR monitor, TV, or projector. Each guided screen will pair a fullscreen test pattern with plain-language instructions and an example of what a bad result looks like.

EyeCal is designed for people without a colorimeter. Measuring color accuracy still requires a meter.

> **Project status · Milestone 3**
>
> `EyeCal.exe` guides you through SDR setup, RGB range, brightness, white clipping, gamma, and sharpness. Each test shows a procedural example of what a bad result looks like.

## Try the Windows prototype

You’ll need Windows 10 or 11 and a Vulkan 1.3 graphics driver. Set Windows to the display’s recommended resolution, then build the executable below or download `EyeCal-Windows-x64` from a successful Windows run in [GitHub Actions](https://github.com/Gunlaus/EyeCal/actions/workflows/build.yml). Unzip the artifact and double-click `EyeCal.exe`.

| Key | What it does |
| --- | --- |
| **F1** | Show or hide the swapchain format, pixel dimensions, present mode, and HDR status |
| **F11** | Switch fullscreen modes. F1 shows whether Vulkan granted exclusive presentation |
| **F2** | Read the bundled licenses. Use **Page Up** and **Page Down** to move between pages |
| **Left / Right**, **Enter** | Go back or forward through the guide |
| **Up / Down** | Choose an answer or menu item |
| **F3** | Cycle text sizes |
| **Esc** | Open the menu to resume or exit |

The same actions are available in the on-screen Back, Next, and Menu buttons. Click a choice or press its number to answer. Scroll down to advance, scroll up or right-click to go back. Move the mouse to reveal the cursor. It hides after two seconds of inactivity and stays visible on the menu. Tab and Shift+Tab move between available buttons, and Enter activates the focused button.

## Calibration checks

- **Set black and white levels.** Find crushed shadows, clipped highlights, and full-range versus limited-range mismatches.
- **Tune gamma and sharpness.** Compare small checkerboards with solid greys, then use fine lines to spot blur, halos, and overscan.
- **Planned: investigate banding.** Compare ramps with and without dithering, switch between 8-bit and 10-bit paths where supported, and check whether banding is concentrated in the shadows.
- **Planned: check uniformity.** Use flat fields to look for uneven brightness and bad pixels, then save a plain-text summary.

## Build from source

Install Visual Studio with **Desktop development with C++**, CMake 3.24 or newer, and the Vulkan SDK with `glslc`. CMake fetches pinned build dependencies.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The executable is at `build/Release/EyeCal.exe`. SDL, volk, shaders, the font atlas, and the English guide are built in. The app uses the system graphics driver and makes no network requests.

<details>
<summary>Optional Linux build and CPU-only checks</summary>

Install a C++20 compiler, CMake, `glslc`, and SDL’s X11 development dependencies. Linux supports borderless fullscreen and reports HDR status as unknown.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSDL_WAYLAND=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For CPU checks without SDL or a shader compiler, add `-DEYECAL_BUILD_APP=OFF` when configuring.

</details>

[Architecture](docs/ARCHITECTURE.md) · [Pattern definitions](docs/PATTERNS.md) · [Verified platform behavior](docs/VERIFIED.md) · [On-device checklist](docs/ON_DEVICE_CHECKLIST.md) · [Third-party notices](THIRD_PARTY.md)
