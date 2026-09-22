#!/usr/bin/env python3
"""Generates seamless (toroidally-wrapped) procedural textures for road and
sidewalk surfaces, since those tile edge-to-edge across many quads and any
AI-generated "seamless" texture tends to show visible seams at that scale.
"""

import math
import random

from PIL import Image, ImageDraw, ImageFilter

SIZE = 256


def wrapped_noise(size, cell, seed):
    """Value noise that tiles seamlessly by sampling on a torus."""
    rnd = random.Random(seed)
    cells = size // cell
    grid = [[rnd.random() for _ in range(cells)] for _ in range(cells)]

    def lerp(a, b, t):
        t = t * t * (3 - 2 * t)
        return a + (b - a) * t

    out = Image.new("F", (size, size))
    px = out.load()
    for y in range(size):
        gy = y / cell
        gy0 = int(gy) % cells
        gy1 = (gy0 + 1) % cells
        fy = gy - int(gy)
        for x in range(size):
            gx = x / cell
            gx0 = int(gx) % cells
            gx1 = (gx0 + 1) % cells
            fx = gx - int(gx)
            v00 = grid[gy0][gx0]
            v10 = grid[gy0][gx1]
            v01 = grid[gy1][gx0]
            v11 = grid[gy1][gx1]
            top = lerp(v00, v10, fx)
            bot = lerp(v01, v11, fx)
            px[x, y] = lerp(top, bot, fy)
    return out


def make_road(path):
    img = Image.new("RGB", (SIZE, SIZE), (46, 46, 50))
    n1 = wrapped_noise(SIZE, 8, 1)
    n2 = wrapped_noise(SIZE, 32, 2)
    px = img.load()
    for y in range(SIZE):
        for x in range(SIZE):
            base = 44 + n1.getpixel((x, y)) * 14 + n2.getpixel((x, y)) * 10
            r = int(base) + 2
            g = int(base) + 2
            b = int(base) + 5
            px[x, y] = (max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b)))

    # A few tireish streaks (wrapped so they cross edges cleanly).
    draw = ImageDraw.Draw(img)
    rnd = random.Random(7)
    for _ in range(10):
        y = rnd.randint(0, SIZE - 1)
        shade = rnd.randint(-14, -4)
        length = rnd.randint(60, 160)
        x0 = rnd.randint(0, SIZE - 1)
        for dx in range(length):
            x = (x0 + dx) % SIZE
            wob = int(2 * math.sin(dx * 0.15 + y))
            yy = (y + wob) % SIZE
            c = img.getpixel((x, yy))
            img.putpixel((x, yy), tuple(max(0, ch + shade) for ch in c))

    img = img.filter(ImageFilter.GaussianBlur(0.4))
    img.save(path)


def make_sidewalk(path):
    img = Image.new("RGB", (SIZE, SIZE), (120, 118, 112))
    n1 = wrapped_noise(SIZE, 6, 11)
    px = img.load()
    for y in range(SIZE):
        for x in range(SIZE):
            base = 112 + n1.getpixel((x, y)) * 18
            px[x, y] = (int(base) + 6, int(base) + 4, int(base))

    draw = ImageDraw.Draw(img)
    step = SIZE // 4
    for i in range(0, SIZE + 1, step):
        draw.line([(i % SIZE, 0), (i % SIZE, SIZE)], fill=(70, 68, 64), width=3)
        draw.line([(0, i % SIZE), (SIZE, i % SIZE)], fill=(70, 68, 64), width=3)

    img = img.filter(ImageFilter.GaussianBlur(0.3))
    img.save(path)


if __name__ == "__main__":
    import sys

    outdir = sys.argv[1] if len(sys.argv) > 1 else "."
    make_road(f"{outdir}/tex_road.png")
    make_sidewalk(f"{outdir}/tex_sidewalk.png")
    print("done")
