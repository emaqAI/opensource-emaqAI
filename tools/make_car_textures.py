#!/usr/bin/env python3
"""Converts the approved car livery PNGs (tools/texture_src/car_*.png) into
4bpp-CLUT .tim files. 4bpp is enough headroom for these (4-5 flat colors
each) and only needs a 64-real-pixel-wide VRAM page, vs 128 for 8bpp -
see the VRAM map comment in make_game_textures.py; these all sit in the
y=256 row, right after the wall_glass/roof pages end at x=640, packed
64 pixels wide each (640, 704, 768, 832, 896, ...).
"""

import struct
import sys

from PIL import Image

TEX_SIZE = 64

# name -> (source png, VRAM pixel x, VRAM pixel y, CLUT y)
CAR_TEXTURES = {
    "tex_car_stripe": ("A_stripe.png", 640, 256, 112),
    "tex_car_taxi": ("B_taxi.png", 704, 256, 114),
    "tex_car_police": ("C_police.png", 768, 256, 116),
    "tex_car_fire": ("E_fire.png", 832, 256, 118),
    "tex_car_ambulance": ("F_ambulance.png", 896, 256, 120),
}


def to_ps1_color(r, g, b):
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


def write_tim_4bpp(path, indexed_img: Image.Image, pix_x, pix_y, clut_y):
    assert indexed_img.mode == "P"
    w, h = indexed_img.size
    assert w == TEX_SIZE and h == TEX_SIZE

    palette = indexed_img.getpalette()
    clut_entries = []
    for i in range(16):
        r = palette[i * 3] if i * 3 < len(palette) else 0
        g = palette[i * 3 + 1] if i * 3 < len(palette) else 0
        b = palette[i * 3 + 2] if i * 3 < len(palette) else 0
        clut_entries.append(to_ps1_color(r, g, b))

    idx = list(indexed_img.get_flattened_data()) if hasattr(indexed_img, "get_flattened_data") else list(indexed_img.getdata())

    packed = bytearray()
    for row in range(h):
        for col in range(0, w, 2):
            lo = idx[row * w + col] & 0xF
            hi = idx[row * w + col + 1] & 0xF
            packed.append(lo | (hi << 4))

    out = bytearray()
    out += struct.pack("<II", 0x00000010, 0x8)  # magic, flags: 4bpp (0) | has-clut (8)

    clut_data = struct.pack("<%dH" % len(clut_entries), *clut_entries)
    clut_block_len = 4 + 2 + 2 + 2 + 2 + len(clut_data)
    out += struct.pack("<I", clut_block_len)
    out += struct.pack("<hhhh", 384, clut_y, 16, 1)
    out += clut_data

    pixel_block_len = 4 + 2 + 2 + 2 + 2 + len(packed)
    out += struct.pack("<I", pixel_block_len)
    out += struct.pack("<hhhh", pix_x, pix_y, w // 4, h)
    out += bytes(packed)

    with open(path, "wb") as f:
        f.write(out)
    print(f"wrote {path}: {len(out)} bytes, pixel @ ({pix_x},{pix_y}) clut @ (384,{clut_y})")


def main():
    srcdir = sys.argv[1] if len(sys.argv) > 1 else "tools/texture_src"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "src/textures"

    for name, (src, px, py, cy) in CAR_TEXTURES.items():
        img = Image.open(f"{srcdir}/{src}").convert("RGB")
        assert img.size == (TEX_SIZE, TEX_SIZE), f"{src} must be {TEX_SIZE}x{TEX_SIZE}"
        indexed = img.quantize(colors=16, method=Image.MEDIANCUT, dither=Image.NONE)
        write_tim_4bpp(f"{outdir}/{name}.tim", indexed, px, py, cy)


if __name__ == "__main__":
    main()
