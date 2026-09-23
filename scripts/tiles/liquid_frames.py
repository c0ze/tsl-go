#!/usr/bin/env python3
"""Palette-cycled animation frames for the liquid tiles (water, lava).

  liquid_frames.py base.png outdir name --ramp water|lava [--frames 4]

The base tile's pixels are ranked by luminance into `levels` bands; each
frame shifts the band -> colour mapping by one step along the ramp, so the
highlights travel through the surface (classic colour cycling). Lava reuses
the water tile's shapes under a molten ramp. The frames are assembled,
timed, and exported in Aseprite (assets/tiles/anim/*.aseprite).
"""
import argparse
import os

from PIL import Image

RAMPS = {
    # dark -> light
    "water": ["#08162a", "#0b1f38", "#0f2946", "#143455", "#1a4166", "#224f79",
              "#2f628f", "#4379a6", "#6a9cc4", "#a8cfe8"],
    "lava": ["#1e0603", "#330904", "#4c0f06", "#6a1707", "#8e2309", "#b3340b",
             "#d24d10", "#ea7019", "#f89a2c", "#ffd35e"],
}


def hex_rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


def frames(base, ramp, n):
    im = Image.open(base).convert("RGB")
    lum = im.convert("L")
    levels = len(ramp)
    # Band edges from the tile's own luminance quantiles (so any source
    # contrast works), skewed so the bright bands are narrow: most of the
    # surface stays deep and dark, only thin crests catch the light.
    hist = sorted(lum.get_flattened_data())
    edges = [hist[min(len(hist) - 1, int(len(hist) * (1 - (1 - k / levels) ** 1.7)))]
             for k in range(1, levels)]
    band = lum.point(lambda v: sum(v > e for e in edges))
    cols = [hex_rgb(c) for c in ramp]
    out = []
    for f in range(n):
        # Only the upper half of the ramp cycles: the deep shades stay put,
        # the lit crests shimmer (a full cycle would strobe).
        def colour(b, f=f):
            if b < levels // 2:
                return cols[b]
            span = levels - levels // 2
            return cols[levels // 2 + (b - levels // 2 + f) % span]
        fr = Image.new("RGB", im.size)
        fp, bp = fr.load(), band.load()
        for y in range(im.height):
            for x in range(im.width):
                fp[x, y] = colour(bp[x, y])
        out.append(fr)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("base")
    ap.add_argument("outdir")
    ap.add_argument("name")
    ap.add_argument("--ramp", choices=RAMPS, required=True)
    ap.add_argument("--frames", type=int, default=4)
    a = ap.parse_args()
    os.makedirs(a.outdir, exist_ok=True)
    for i, fr in enumerate(frames(a.base, RAMPS[a.ramp], a.frames)):
        fr.save(os.path.join(a.outdir, f"{a.name}_{i}.png"))


if __name__ == "__main__":
    main()
