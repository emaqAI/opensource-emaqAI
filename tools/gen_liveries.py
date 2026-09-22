#!/usr/bin/env python3
"""Candidate car-body livery textures, 64x64, for approval before wiring
into the game. Pure procedural (crisp geometric patterns read far better
than AI photo textures at this resolution)."""

from PIL import Image, ImageDraw

S = 64


def base(color):
    return Image.new("RGB", (S, S), color)


def variant_a_stripe():
    """Current red body + a single white racing stripe down the middle."""
    img = base((190, 40, 40))
    d = ImageDraw.Draw(img)
    d.rectangle([S // 2 - 6, 0, S // 2 + 5, S], fill=(235, 235, 230))
    d.rectangle([S // 2 - 2, 0, S // 2 + 1, S], fill=(190, 40, 40))
    return img


def variant_b_taxi():
    """Taxi yellow with a black checker stripe."""
    img = base((225, 185, 30))
    d = ImageDraw.Draw(img)
    band_y0, band_y1 = S // 2 - 6, S // 2 + 6
    d.rectangle([0, band_y0, S, band_y1], fill=(20, 20, 20))
    cell = 6
    for x in range(0, S, cell):
        if (x // cell) % 2 == 0:
            d.rectangle([x, band_y0, x + cell - 1, band_y1], fill=(230, 230, 225))
    return img


def variant_c_police():
    """Silver body, dark navy-blue band, and a light green "Battenburg"
    style reflective checker stripe (the classic PL police look)."""
    img = base((196, 200, 206))  # silver
    d = ImageDraw.Draw(img)

    navy = (18, 40, 110)
    green = (150, 255, 90)  # light, fluorescent-looking green

    # Navy band across the doors.
    d.rectangle([0, S // 2 - 15, S, S // 2 + 15], fill=navy)

    # Reflective green checker stripe through the middle of the band.
    cell = 6
    y0, y1 = S // 2 - 5, S // 2 + 4
    for x in range(0, S, cell):
        if (x // cell) % 2 == 0:
            d.rectangle([x, y0, x + cell - 1, y1], fill=green)
        else:
            d.rectangle([x, y0, x + cell - 1, y1], fill=navy)

    return img


def variant_e_fire():
    """Fire brigade: red body with a white/red "Battenburg" reflective
    checker band, same grammar as the police livery but red instead of
    navy/green - the classic European fire-engine look."""
    img = base((200, 30, 30))  # fire-engine red
    d = ImageDraw.Draw(img)

    white = (240, 240, 235)

    # White band across the doors.
    d.rectangle([0, S // 2 - 15, S, S // 2 + 15], fill=white)

    # Red/white checker stripe through the middle of the band.
    cell = 6
    y0, y1 = S // 2 - 5, S // 2 + 4
    for x in range(0, S, cell):
        if (x // cell) % 2 == 0:
            d.rectangle([x, y0, x + cell - 1, y1], fill=(200, 30, 30))
        else:
            d.rectangle([x, y0, x + cell - 1, y1], fill=white)

    return img


def variant_f_ambulance():
    """Ambulance: white body with a red door band and a small red cross,
    same grammar as the other emergency liveries but white instead of a
    saturated body color."""
    img = base((240, 240, 238))  # white
    d = ImageDraw.Draw(img)

    red = (200, 30, 30)

    # Red band across the doors.
    d.rectangle([0, S // 2 - 12, S, S // 2 + 12], fill=red)

    # A simple red cross centered in the band.
    cx, cy = S // 2, S // 2
    d.rectangle([cx - 3, cy - 9, cx + 2, cy + 8], fill=(240, 240, 238))
    d.rectangle([cx - 9, cy - 3, cx + 8, cy + 2], fill=(240, 240, 238))

    return img


def variant_d_muscle():
    """Dark body, twin white racing stripes (muscle-car style)."""
    img = base((35, 35, 40))
    d = ImageDraw.Draw(img)
    d.rectangle([S // 2 - 12, 0, S // 2 - 6, S], fill=(235, 235, 230))
    d.rectangle([S // 2 + 5, 0, S // 2 + 11, S], fill=(235, 235, 230))
    return img


VARIANTS = {
    "A_stripe": variant_a_stripe,
    "B_taxi": variant_b_taxi,
    "C_police": variant_c_police,
    "D_muscle": variant_d_muscle,
    "E_fire": variant_e_fire,
    "F_ambulance": variant_f_ambulance,
}


def main():
    imgs = {name: fn() for name, fn in VARIANTS.items()}
    for name, img in imgs.items():
        img.save(f"tools/texture_src/{name}.png")

    # Comparison sheet: each variant at native 64x64 and upscaled (nearest,
    # to preview exactly how it'll look blocky in-game) side by side.
    pad = 16
    cell_w = 64 * 4 + pad
    cell_h = 64 * 4 + 40
    sheet = Image.new("RGB", (cell_w * len(imgs) + pad, cell_h + pad * 2), (30, 30, 34))
    d = ImageDraw.Draw(sheet)
    x = pad
    for name, img in imgs.items():
        big = img.resize((64 * 4, 64 * 4), Image.NEAREST)
        sheet.paste(big, (x, pad + 24))
        d.text((x, pad), name, fill=(230, 230, 230))
        x += cell_w
    sheet.save("tools/texture_src/comparison_sheet.png")
    print("done")


if __name__ == "__main__":
    main()
