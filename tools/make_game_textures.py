#!/usr/bin/env python3
"""Crops/quantizes source texture images and writes them out as PS1 .tim
files (8bpp CLUT), placed at fixed, non-overlapping, page-aligned VRAM
coordinates.

VRAM map used here (framebuffers occupy x:0-320,y:0-480; the debug font
sits near x:960,y:0 - see src/main.c). 8bpp texture pages are 128 real
pixels wide x 256 tall, so pixel data is placed on 128/256-aligned
origins (one texture per page, well clear of the framebuffers and font):

    road          @ (384,  0)      sidewalk      @ (512,  0)
    wall_concrete @ (640,  0)      wall_brick    @ (768,  0)
    wall_glass    @ (384,256)      roof          @ (512,256)

Each texture is only 64x64 real pixels, so within every page rows 64-255
are unused; CLUTs (256x1 real pixels each) are tucked into that dead
space at x=384, y=100..110, comfortably clear of any pixel data (which
only occupies y:0-63 and y:256-319) and of each other.
"""

import struct
import sys

from PIL import Image

TEX_SIZE = 64

# name -> (source image, crop box or None, VRAM pixel x, VRAM pixel y, CLUT row y)
TEXTURES = {
    "tex_road": ("tex_road.png", None, 384, 0, 100),
    "tex_sidewalk": ("tex_sidewalk.png", None, 512, 0, 102),
    "tex_wall_concrete": ("raw_wall_concrete.jpg", (20, 110, 340, 430), 640, 0, 104),
    "tex_wall_brick": ("raw_wall_brick.jpg", (0, 0, 460, 440), 768, 0, 106),
    "tex_wall_glass": ("raw_wall_glass3.jpg", (0, 0, 460, 440), 384, 256, 108),
    "tex_roof": ("raw_roof2.jpg", (0, 0, 440, 440), 512, 256, 110),
}


def to_ps1_color(r, g, b):
    r5, g5, b5 = r >> 3, g >> 3, b >> 3
    return r5 | (g5 << 5) | (b5 << 10)


def write_tim(path, indexed_img: Image.Image, pix_x, pix_y, clut_y):
    assert indexed_img.mode == "P"
    w, h = indexed_img.size
    assert w == TEX_SIZE and h == TEX_SIZE

    palette = indexed_img.getpalette()  # flat [r,g,b, r,g,b, ...], 256 entries (may be padded)
    clut_entries = []
    for i in range(256):
        r = palette[i * 3] if i * 3 < len(palette) else 0
        g = palette[i * 3 + 1] if i * 3 < len(palette) else 0
        b = palette[i * 3 + 2] if i * 3 < len(palette) else 0
        clut_entries.append(to_ps1_color(r, g, b))

    pixels = list(indexed_img.getdata())  # row-major indices, one byte each

    out = bytearray()
    out += struct.pack("<II", 0x00000010, 0x9)  # magic, flags: 8bpp (1) | has-clut (8)

    # CLUT block
    clut_data = struct.pack("<%dH" % len(clut_entries), *clut_entries)
    clut_block_len = 4 + 2 + 2 + 2 + 2 + len(clut_data)
    out += struct.pack("<I", clut_block_len)
    out += struct.pack("<hhhh", 384, clut_y, 256, 1)  # x, y, w(entries), h(palettes)
    out += clut_data

    # Pixel block. x/y are real VRAM pixel (halfword-column) coordinates,
    # unscaled; only the width is expressed in halfwords (2 texels/halfword
    # at 8bpp), matching the on-disk layout of PSn00bSDK's own .tim files.
    pixel_data = bytes(pixels)
    pixel_block_len = 4 + 2 + 2 + 2 + 2 + len(pixel_data)
    out += struct.pack("<I", pixel_block_len)
    out += struct.pack("<hhhh", pix_x, pix_y, w // 2, h)
    out += pixel_data

    with open(path, "wb") as f:
        f.write(out)
    print(f"wrote {path}: {len(out)} bytes, pixel @ ({pix_x},{pix_y}) clut @ (384,{clut_y})")


def main():
    srcdir = sys.argv[1] if len(sys.argv) > 1 else "tools/texture_src"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "src/textures"

    for name, (src, crop, px, py, cy) in TEXTURES.items():
        img = Image.open(f"{srcdir}/{src}").convert("RGB")
        if crop:
            img = img.crop(crop)
        img = img.resize((TEX_SIZE, TEX_SIZE), Image.LANCZOS)

        preview_path = f"{outdir}/{name}_preview.png"
        img.save(preview_path)

        indexed = img.quantize(colors=256, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG)
        write_tim(f"{outdir}/{name}.tim", indexed, px, py, cy)


if __name__ == "__main__":
    main()
