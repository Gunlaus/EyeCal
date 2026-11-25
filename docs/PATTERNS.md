# Pattern definitions

| Milestone | Pattern | Exact input and intended swapchain codes | Rationale and source |
| --- | --- | --- | --- |
| M1 | Grey clear field | R, G and B each receive `64.0f / 255.0f` in the attachment clear. Each 8-bit UNORM channel represents code 64. Alpha is 1.0, represented as code 255. The field fills the swapchain extent when the overlay is hidden on the M1 fullscreen prototype. | A fixed, original clear color exercises presentation without introducing a ramp, transfer function, dithering, or pattern textures. The normalized conversion and load operation follow the [Vulkan fixed-point conversion rules](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedfpconv) and [dynamic rendering attachment specification](https://docs.vulkan.org/refpages/latest/refpages/source/VkRenderingAttachmentInfo.html). |
| M2 | Grey ramp preview | For a region of width `w > 1`, column `x` uses `x / (w - 1)` in R, G and B. Width one uses zero. The 8-bit path covers code values 0–255, rounded to nearest with ties up when dither is off. Alpha is 255. | Exercises the shared quantizer on a linear progression of encoded codes. [Vulkan normalized fixed-point conversion](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedfpconv) |
| M2 | Deliberately coarse thumbnail | The grey ramp shader substitutes `floor(8*x/w) / 7` before quantization. With dither off its eight 8-bit stripe codes are 0, 36, 73, 109, 146, 182, 219, 255 when all stripes are present. | Shows obvious bands as a visual reference next to the main ramp. [Vulkan normalized fixed-point conversion](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedfpconv) |

The ranges above describe swapchain code values. [Windows presentation and driver limitations](VERIFIED.md) also apply.

### Shared quantization

The shader takes display-encoded values and computes `floor(value * maxCode + offset + 0.5)`, clamped to the destination code range. It writes `code / maxCode` to the UNORM attachment. Exact zero and one remain unchanged in every mode. NaN maps to black. The CPU reference returns integer codes and also tests the 10-bit range 0–1023.

| Mode | Offset in destination code steps |
| --- | --- |
| Off | Zero. Default when entering the guide for the first time. |
| Ordered | Centred midpoint ranks from a tiled 8 × 8 Bayer matrix. |
| Blue noise | Centred midpoint ranks from a periodic 16 × 16 void-and-cluster rank texture. The texture is embedded as packed 8-bit texels and sampled by integer pixel coordinates. |
| TPDF | Difference of two uniform hash samples. The offset lies strictly between -1 and 1. The preview uses a fixed seed. |

See [the blue-noise asset origin and licence](../THIRD_PARTY.md) and [Ulichney’s void-and-cluster paper](https://doi.org/10.1117/12.152707).

Flat fields and the synthetic reference scene are scheduled for a later milestone.

## M3 calibration patterns

All values below are 8-bit full-range RGB codes, with dither Off. A strip index is the integer floor of its horizontal pixel coordinate times the strip count divided by region width. Sources describe the signal conventions and transfer functions. The layouts are original.

| Screen | Pixel values and rationale | Source |
| --- | --- | --- |
| 0. Setup | Four neutral vertical swatches at 48, 64, 80, 96. The bad thumbnail repeats 48. This checks visibility of a simple progression. | [Vulkan normalized fixed-point conversion](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedfpconv) |
| 1. Levels | Upper row 0–8 and lower row 247–255 inclusive. Bad example is 0 above and 255 below, illustrating clipping of extremes. | [Microsoft nominal-range definitions](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/ne-mfobjects-mfnominalrange) |
| 2. Black | Centre half of region has codes 0, 1, 2, 3, 4, 6, 8, 12, 16, 24 over a code-0 surround. Bad example zeros values through 8. Full-range 8-bit RGB starts at 0, so the EyeCal PLUGE-style comparison uses reference black and above-black steps. | [Microsoft nominal-range definitions](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/ne-mfobjects-mfnominalrange) |
| 3. White | Codes 235, 240, 245, 247, 249, 251, 253, 254, 255. Bad example sets every value from 247 onward to 255. | [Microsoft nominal-range definitions](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/ne-mfobjects-mfnominalrange) |
| 4. Gamma | Three 2 × 2 tiles contain 1, 2 and 3 white pixels respectively, with codes 0 and 255. Solids are nearest codes to the inverse display transfer at linear fractions 0.25, 0.5 and 0.75. sRGB yields 137, 188, 225. Gamma 2.2 yields 136, 186, 224. Ideal-black BT.1886 yields 143, 191, 226. The bad example adds 0.14 encoded value before quantization, clamped to white. | [W3C sRGB transfer function](https://www.w3.org/TR/css-color-4/#color-conversion-code) · [ITU-R BT.1886](https://www.itu.int/rec/R-REC-BT.1886) |
| 5. Sharpness | Alternating 64/192 single-pixel lines and a clean 64/192 edge. Border and outer-screen marks use code 224. Corner marks use 255. Bad example substitutes 16 and 240 in the two columns adjacent to the edge on each side. | [Vulkan fragment coordinates](https://docs.vulkan.org/refpages/latest/refpages/source/FragCoord.html) |

## M4 banding diagnostic

| Part | Exact signal and interpretation |
| --- | --- |
| 6a. Baseline | Vertical encoded ramp from 0 to 1 across the region, with code `round(255*y/(h-1))` on the 8-bit path and no dither. One row is black. The coarse thumbnail uses 8 evenly spaced code levels. |
| 6b. Dither comparison | The same normalized ramp is shared row-for-row across the two sides. The left side uses no dither and the right uses the selected ordered, blue-noise, or TPDF offset before quantization. The divider changes the split column only. |
| 6c. Depth | The same horizontal encoded ramp and region are drawn for both passes with dither off. On an 8-bit surface the code maximum is 255, and on a 10-bit surface it is 1023. A surface format states attachment precision. Actual scanout depth requires independent verification. |
| 6d. Shadows | Upper region maps linear light 0–0.20 through the sRGB transfer function. Lower region maps linear light 0.20–0.40. Both span the full width with dither off. At 8-bit the endpoint pairs are 0–124 and 124–170. In motion mode, replace normalized horizontal position `x` with `1 - abs(2 * fract(0.5 * x + phase) - 1)`. Phase moves at one cycle per 16 seconds on a monotonic clock. This is a visually moving stimulus for observing temporal artifacts. |

Underlying conventions are documented by [Khronos on swapchain format pairs](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPhysicalDeviceSurfaceFormatsKHR.html), the [Vulkan fixed-point conversion rules](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedfpconv), and the [W3C sRGB conversion code](https://www.w3.org/TR/css-color-4/#color-conversion-code).

## Live SDR colour and HDR10

- **Dither on/off.** The first banding screen toggles blue noise in the main vertical ramp. It uses the same geometry, code range and seed for both states. The coarse thumbnail stays undithered.
- **Greyscale tint.** Neutral RGB triplets 16, 32, 64, 96, 128, 176, 224 and 255. The example adds `(0.08, 0, -0.04) × sin(π × grey)` in normalized encoded RGB. White-balance accuracy requires a meter.
- **Saturation and channel clipping.** Six RGB/CMY rows mix code-128 neutral with their respective primary/secondary at strengths 0, 64, 128, 166, 191, 217, 242 and 255 over 255. The example clips its last three columns to full saturation. The separate RGB clipping test uses 224, 232, 240, 246, 250, 253 and 255 in one channel at a time. These are signal comparisons rather than measured gamut/saturation targets.
- **Uniformity.** Flat 8-bit code values 13, 51, 128, 255, red, green, blue and black in the pattern region.
- **HDR clipping and shadows.** BT.2100 PQ / BT.2020 neutral values with a code-zero surround. Highlights use a 20%-height strip with 100, 200, 400, 600, 1000, 1500, 2000 and 4000 cd/m² targets. Shadow patches use a half-height strip with 0, 0.005, 0.01, 0.02, 0.05, 0.1, 0.5 and 1 cd/m² targets. At 10-bit, several low levels may be hard to distinguish on a given display.
- **HDR reference white and window response.** Reference white uses 80, 100, 203 and 300 cd/m² in a central band. The area test uses the same encoded 600 cd/m² patch at 2%, 10% and 25% of the available pattern region. All HDR levels use the same PQ inverse EOTF before 10-bit UNORM quantization.

See [ITU-R BT.2100](https://www.itu.int/rec/R-REC-BT.2100) for PQ and BT.2020, and [Khronos colour space semantics](https://docs.vulkan.org/refpages/latest/refpages/source/VkColorSpaceKHR.html) for the Vulkan HDR10/ST 2084 surface.
