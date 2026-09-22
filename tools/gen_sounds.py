#!/usr/bin/env python3
"""Synthesizes the game's 3 sound effects (engine loop, siren loop, crash
one-shot) as raw waveforms and encodes them to PS1 SPU-ADPCM (.vag), since
there's no source audio to record and no license-free sample library to
pull from - simple synthesized tones/noise read fine for a PS1-era engine
hum/siren/impact anyway.

The encoder always uses ADPCM predictor 0 (no linear prediction, just a
per-28-sample-block shift/quantize to 4 bits) - a real, spec-compliant
SPU-ADPCM mode, just the simplest one. It's a bit more "crunchy" than a
full predictor-search encoder, which suits lo-fi PS1 SFX fine and avoids
needing to port a real encoder's predictor search from scratch.
"""

import math
import random
import struct
import sys

SAMPLE_RATE = 8000
BLOCK_SAMPLES = 28


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def encode_adpcm(samples, loop=False):
    """samples: list of int16. Returns the raw SPU-ADPCM byte stream."""
    pad = (-len(samples)) % BLOCK_SAMPLES
    samples = samples + [0] * pad

    out = bytearray()
    nblocks = len(samples) // BLOCK_SAMPLES

    for bi in range(nblocks):
        block = samples[bi * BLOCK_SAMPLES:(bi + 1) * BLOCK_SAMPLES]
        max_abs = max(abs(s) for s in block) or 1

        shift = 12
        for s in range(0, 13):
            ok = True
            for v in block:
                q = v >> s
                if q < -8 or q > 7:
                    ok = False
                    break
            if ok:
                shift = s
                break

        nibbles = []
        for v in block:
            q = clamp(v >> shift, -8, 7)
            nibbles.append(q & 0xF)

        flag = 0
        if bi == 0 and loop:
            flag |= 0x04  # loop start marker
        if bi == nblocks - 1:
            flag |= 0x01  # end of sample
            if loop:
                flag |= 0x02  # repeat (jump back to loop-start block)

        out.append((0 << 4) | shift)  # filter 0, this shift
        out.append(flag)

        for i in range(0, BLOCK_SAMPLES, 2):
            lo = nibbles[i]
            hi = nibbles[i + 1]
            out.append(lo | (hi << 4))

    return bytes(out)


def write_vag(path, name, adpcm_body, sample_rate):
    header = struct.pack(
        ">4sIII I 10x H16s",
        b"VAGp", 0x20, 0, len(adpcm_body), sample_rate,
        0, name.encode("ascii")[:16].ljust(16, b"\0"),
    )
    with open(path, "wb") as f:
        f.write(header)
        f.write(adpcm_body)
    print(f"wrote {path}: {len(header) + len(adpcm_body)} bytes ({len(adpcm_body)} ADPCM)")


def synth_engine():
    """A short, phase-continuous low buzzy tone loop - the base engine hum,
    pitch-shifted at runtime by scaling the SPU playback rate with speed."""
    freq = 90.0
    cycles = 6
    n = int(SAMPLE_RATE * cycles / freq)
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        # A slightly dirty tone (fundamental + a couple of harmonics) reads
        # more like an engine than a pure sine.
        v = (
            0.55 * math.sin(2 * math.pi * freq * t)
            + 0.30 * math.sin(2 * math.pi * freq * 2 * t)
            + 0.15 * math.sin(2 * math.pi * freq * 3 * t)
        )
        samples.append(int(clamp(v * 9000, -32000, 32000)))
    return samples


def synth_siren():
    """Two-tone alternating wail (EU "nee-naw" siren), looped."""
    tone_a, tone_b = 500.0, 720.0
    seg = 0.5  # seconds per tone
    n_seg = int(SAMPLE_RATE * seg)
    samples = []
    for tone in (tone_a, tone_b):
        for i in range(n_seg):
            t = i / SAMPLE_RATE
            v = math.sin(2 * math.pi * tone * t)
            samples.append(int(clamp(v * 11000, -32000, 32000)))
    return samples


def synth_crash():
    """A short burst of decaying noise - a generic impact/crash thud."""
    dur = 0.35
    n = int(SAMPLE_RATE * dur)
    random.seed(7)
    samples = []
    for i in range(n):
        decay = (1.0 - i / n) ** 2
        v = (random.random() * 2 - 1) * decay
        samples.append(int(clamp(v * 20000, -32000, 32000)))
    return samples


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "src/sounds"

    write_vag(f"{outdir}/engine.vag", "engine", encode_adpcm(synth_engine(), loop=True), SAMPLE_RATE)
    write_vag(f"{outdir}/siren.vag", "siren", encode_adpcm(synth_siren(), loop=True), SAMPLE_RATE)
    write_vag(f"{outdir}/crash.vag", "crash", encode_adpcm(synth_crash(), loop=False), SAMPLE_RATE)


if __name__ == "__main__":
    main()
