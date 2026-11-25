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

Levels, PLUGE-style black level, white clip, gamma, sharpness, banding, flat fields and the synthetic reference scene are scheduled for later milestones. Add exact code values, rationale and citations when introducing each pattern.
