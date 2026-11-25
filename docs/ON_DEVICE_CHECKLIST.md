# On-device checklist

Every item below needs on-device checking. The build and CPU tests provide no visual verification.

## M1 fullscreen and diagnostics

- [ ] Double-click a standalone release exe on Windows 10 and 11 with a Vulkan 1.3 driver. Confirm fullscreen opens with a neutral grey field and a readable F1 overlay. Test the startup error on systems without a compatible driver.
- [ ] Check F1 hides and restores diagnostics. Compare the reported extent to Windows at recommended resolution, different desktop DPI scales, and on a second monitor. Confirm the format is BGRA8 or RGBA8 UNORM and the present mode is FIFO.
- [ ] Compare borderless and F11 modes. Confirm the overlay accurately distinguishes a granted Vulkan exclusive mode from display-mode-only fullscreen. Toggle repeatedly, Alt-Tab, restore, then exit. Confirm the desktop mode and normal input return. Check on hardware with and without the Vulkan extension.
- [ ] Enable and disable Windows HDR. Confirm the HDR warning appears within two seconds while active, including with F1 hidden. Confirm the overlay's output status on mixed HDR and SDR displays. Check Advanced Color separately where supported.
- [ ] Use F2 and Page Up/Page Down to inspect the embedded notices. Check legibility from typical projector viewing distance.
- [ ] Compare the grey field in borderless and exclusive modes with driver image enhancements, display scaling and overscan varied. Capture the presentation path or external output if quantifying changes. Confirm pixel mapping with the M3 sharpness grid when available.

## M2 guide and ramp preview

- [ ] Read the intro at normal viewing distance. Check the HDR and auto colour management instruction on a Windows display that exposes both settings.
- [ ] Navigate with Left, Right and Enter. Choose answers with Up and Down, go back, and check that each screen retains its own answer. Open the Esc menu, resume, then exit.
- [ ] Cycle text size with F3 at low and high resolutions. Check that controls remain readable and that the pattern and bad-example thumbnail sit outside every text panel, including with F1 visible.
- [ ] On the ramp preview, check Off, Ordered, Blue noise and TPDF. Confirm the thumbnail stays coarse, the main ramp has black and white endpoints, and the dither pattern is stable across redraws.
- [ ] Compare the GPU ramp with the CPU reference using a capture that preserves swapchain code values. Check F1 and F11 independently of the guide, including exclusive-mode loss and recovery.

## Mouse and keyboard controls

- [ ] Click Back, Next, Menu, and each answer on the intro and ramp preview. Confirm edge buttons stay disabled where appropriate, and that a press dragged outside the original button does not trigger it.
- [ ] Hover, press and release each button. Check Tab and Shift+Tab focus, Enter activation, number-key choices, and the original arrows and shortcuts. Confirm wheel down/up advances/goes back and right-click goes back.
- [ ] Confirm the cursor starts hidden over the guide, appears on movement, and hides after two idle seconds. Leave the menu open beyond two seconds and check that the cursor stays visible. Repeat on the summary screen when it is implemented.
- [ ] Repeat at multiple F3 sizes and Windows DPI settings, in both fullscreen modes and after Alt-Tab or monitor changes. Confirm the hit areas match the visible buttons, and that controls never cover either ramp.
- [ ] Try a physical wheel and a touchpad, including natural scrolling. Confirm the cursor returns normally when leaving the app.

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
| 6c. Bit-depth probe | Switch between 8- and 10-bit swapchains on capable hardware. Test unsupported hardware and the borderless inconclusive result. Verify the exclusive-only rule before comparing against known signal-chain settings. |
| 6d. Dark ramp | Compare near-black and mid-grey gradients with dark processing controls varied. Check the moving ramp for temporal artifacts. |
| 7. Uniformity | Cycle every flat field and verify fullscreen coverage. Check for clouding, dirty-screen effect, vignetting and pixel defects. |
| 8. Summary | Compare answers and statuses to the entered results. Inspect the synthetic shadow and highlight details and open the exported plain-text report. |

## Visual chrome

- [ ] Read the intro, menu and pattern controls from 3 m on a projector at each usable text size. Check focus, disabled buttons, captions, step count and progress.
- [ ] Move between a brighter UI screen and a dark pattern screen. Check for a visible brightness shift on the display. Repeat on an OLED or projector with automatic brightness controls in its normal configuration.
- [ ] Toggle Reduce motion and compare screen and menu transitions. Confirm hover and focus inside the bands remain instantaneous when a pattern is visible, and that no UI covers any pattern pixels.
- [ ] Check the icon in Windows Explorer and the window title in task switching.
