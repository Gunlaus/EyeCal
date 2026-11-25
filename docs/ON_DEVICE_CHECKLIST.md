# On-device checklist

Every item below needs on-device checking. The build and CPU tests provide no visual verification.

## M1 fullscreen and diagnostics

- [ ] Double-click a standalone release exe on Windows 10 and 11 with a Vulkan 1.3 driver. Confirm fullscreen opens with a neutral grey field and a readable F1 overlay. Test the startup error on systems without a compatible driver.
- [ ] Check F1 hides and restores diagnostics. Compare the reported extent to Windows at recommended resolution, different desktop DPI scales, and on a second monitor. Confirm the format is BGRA8 or RGBA8 UNORM and the present mode is FIFO.
- [ ] Compare borderless and F11 modes. Confirm the overlay accurately distinguishes a granted Vulkan exclusive mode from display-mode-only fullscreen. Toggle repeatedly, Alt-Tab, restore, then exit. Confirm the desktop mode and normal input return. Check on hardware with and without the Vulkan extension.
- [ ] Enable and disable Windows HDR. Confirm the HDR warning appears within two seconds while active, including with F1 hidden. Confirm the overlay's output status on mixed HDR and SDR displays. Check Advanced Color separately where supported.
- [ ] Use F2 and Page Up/Page Down to inspect the embedded notices. Check legibility from typical projector viewing distance.
- [ ] Compare the grey field in borderless and exclusive modes with driver image enhancements, display scaling and overscan varied. Capture the presentation path or external output if quantifying changes. Confirm pixel mapping with the M3 sharpness grid when available.

## Future calibration screens

| Screen | What to confirm on real hardware once implemented |
| --- | --- |
| 0. Intro | Read the checklist at normal viewing distance. Change display type and confirm the later hints and gamma default follow the selection. Verify the HDR warning and SDR guidance. |
| 1. Levels | Deliberately mismatch full and limited RGB settings on a TV. Confirm the expected near-black and near-white steps disappear or look washed out and the hints identify the relevant controls. |
| 2. Black level | Check near-black step visibility on full and limited output and confirm the instructions handle the absence of below-black headroom in full range. |
| 3. White clip | Raise contrast until the brightest steps merge, then lower it until the top steps separate. |
| 4. Gamma | Compare coarse and fine patch visibility at native resolution and normal viewing distance for all three targets. |
| 5. Sharpness | Verify the 1px lines and border markers. Compare native and scaled modes, overscan off and on, and low and high edge enhancement. |
| 6a. Baseline ramp | Compare the no-dither ramp on a known 8-bit output and confirm its appearance stays stable. |
| 6b. Dither comparison | Move the divider and compare each dither algorithm against the same unchanged ramp. |
| 6c. Bit-depth probe | Switch between 8- and 10-bit swapchains on capable hardware. Test unsupported hardware, then compare against known signal-chain settings. |
| 6d. Dark ramp | Compare near-black and mid-grey gradients with dark processing controls varied. Check the moving ramp for temporal artifacts. |
| 7. Uniformity | Cycle every flat field and verify fullscreen coverage. Check for clouding, dirty-screen effect, vignetting and pixel defects. |
| 8. Summary | Compare answers and statuses to the entered results. Inspect the synthetic shadow and highlight details and open the exported plain-text report. |
