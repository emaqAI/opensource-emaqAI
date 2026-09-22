#!/usr/bin/env python3
"""
Generates Chrome City cover art via the OpenAI Images API.

Setup:
    pip install openai
    export OPENAI_API_KEY="sk-..."   # your NEW key — never paste it into chat/code

Usage:
    python3 tools/generate_cover.py
    python3 tools/generate_cover.py --prompt "custom prompt here" --out cover.png
"""

import argparse
import base64
import os
import sys

from openai import OpenAI

DEFAULT_PROMPT = (
    "Retro PlayStation 1 era video game box art cover, titled 'CHROME CITY' in "
    "bold chunky 1998 video game logo lettering at the top. A gritty neon-lit "
    "night city skyline in the background with low-poly PS1-style skyscrapers, "
    "warm orange and cyan window lights, a hazy orange moon. In the foreground, "
    "a low-poly red muscle car speeding toward the viewer down a dark wet asphalt "
    "street with a single yellow lane line, motion blur streaks. Overall mood: "
    "1990s crime/driving game box art, dramatic rim lighting, dark navy and "
    "magenta color palette, slight CRT scanline texture, portrait orientation, "
    "polished commercial game cover composition."
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prompt", default=DEFAULT_PROMPT, help="Image prompt.")
    parser.add_argument("--out", default="chrome_city_cover.png", help="Output PNG path.")
    parser.add_argument(
        "--size",
        default="1024x1536",
        choices=["1024x1024", "1024x1536", "1536x1024", "auto"],
        help="Image size (portrait by default, matches box-art proportions).",
    )
    parser.add_argument(
        "--model",
        default="gpt-image-1",
        help="Image model. gpt-image-1 is the current OpenAI image model; "
        "use 'dall-e-3' if your account doesn't have gpt-image-1 access.",
    )
    parser.add_argument("--quality", default="high", help="Image quality (gpt-image-1: low/medium/high).")
    args = parser.parse_args()

    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        print(
            "error: OPENAI_API_KEY is not set.\n"
            "  export OPENAI_API_KEY=\"sk-...\"   (your NEW key, never hardcode it here)",
            file=sys.stderr,
        )
        return 1

    client = OpenAI(api_key=api_key)

    kwargs = dict(model=args.model, prompt=args.prompt, size=args.size, n=1)
    if args.model == "gpt-image-1":
        kwargs["quality"] = args.quality

    print(f"Generating with {args.model} ({args.size})...")
    result = client.images.generate(**kwargs)

    image_data = result.data[0]
    if getattr(image_data, "b64_json", None):
        image_bytes = base64.b64decode(image_data.b64_json)
    else:
        # dall-e-3 returns a temporary URL instead of inline base64.
        import urllib.request

        with urllib.request.urlopen(image_data.url) as resp:
            image_bytes = resp.read()

    with open(args.out, "wb") as f:
        f.write(image_bytes)

    print(f"Saved: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
