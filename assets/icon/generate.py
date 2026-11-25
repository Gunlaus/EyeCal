"""Generate the EyeCal Windows icon from geometric shapes using Pillow."""
from pathlib import Path
from PIL import Image, ImageDraw

image = Image.new("RGBA", (256, 256))
draw = ImageDraw.Draw(image)
draw.rounded_rectangle((8, 8, 248, 248), radius=56, fill="#151b23")
draw.rounded_rectangle((44, 55, 212, 178), radius=24, outline="#88b9df", width=12)
for x, grey in [(67, 61), (99, 102), (131, 154), (163, 206)]:
    draw.rectangle((x, 85, x + 26, 148), fill=(grey, grey, grey))
draw.rounded_rectangle((91, 193, 165, 204), radius=6, fill="#88b9df")
image.save(Path(__file__).with_name("EyeCal.ico"), sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])
