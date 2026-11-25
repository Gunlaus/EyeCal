# Pattern definitions

| Milestone | Pattern | Exact input and intended swapchain codes | Rationale and source |
| --- | --- | --- | --- |
| M1 | Grey clear field | R, G and B each receive `64.0f / 255.0f` in the attachment clear. Each 8-bit UNORM channel represents code 64. Alpha is 1.0, represented as code 255. The field fills the swapchain extent when the overlay is hidden and SDR is active. | A fixed, original clear color exercises presentation without introducing a ramp, transfer function, dithering, or pattern textures. The normalized conversion and load operation follow the [Vulkan fixed-point conversion rules](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedfpconv) and [dynamic rendering attachment specification](https://docs.vulkan.org/refpages/latest/refpages/source/VkRenderingAttachmentInfo.html). |

The diagnostic overlay occupies a separate corner of the field. Disable it with F1 for a uniform frame. The table describes swapchain values. [Windows presentation and driver limitations](VERIFIED.md) also apply.

Levels, PLUGE-style black level, white clip, gamma, sharpness, banding, flat fields and the synthetic reference scene are scheduled for later milestones. Add exact code values, rationale and citations when introducing each pattern.
