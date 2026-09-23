#!/usr/bin/env python3
"""Copy the DCSS tiles named in sources.DCSS into assets/tiles/dcss/.

  import_dcss.py "/path/to/Dungeon Crawl Stone Soup Supplemental"

Only the tiles the game uses are committed (CC0; credits in
assets/tiles/README.md). An entry with a hue shift is recoloured on the way
in — the gold dragon turned red, the azure jelly green — so the committed
PNG is exactly what the atlas packs. PNGs in dcss/ that sources.DCSS no
longer names are deleted (and listed), so the atlas can't keep stale tiles.
"""
import colorsys
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from sources import DCSS  # noqa: E402

OUT = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "tiles", "dcss")


def hue_shift(im, degrees):
    im = im.convert("RGBA")
    px = im.load()
    shift = degrees / 360.0
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            if not a:
                continue
            h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            r2, g2, b2 = colorsys.hsv_to_rgb((h + shift) % 1.0, s, v)
            px[x, y] = (round(r2 * 255), round(g2 * 255), round(b2 * 255), a)
    return im


def main():
    root = sys.argv[1]
    os.makedirs(OUT, exist_ok=True)
    missing = []
    for name, (rel, shift) in sorted(DCSS.items()):
        src = os.path.join(root, rel)
        if not os.path.exists(src):
            missing.append(rel)
            continue
        im = Image.open(src).convert("RGBA")
        if shift:
            im = hue_shift(im, shift)
        im.save(os.path.join(OUT, name + ".png"))
    if missing:
        sys.exit("missing in the pack:\n  " + "\n  ".join(missing))
    for f in sorted(os.listdir(OUT)):  # drop tiles no longer named in sources.DCSS
        if f.endswith(".png") and f[:-4] not in DCSS:
            os.remove(os.path.join(OUT, f))
            print("removed", f)
    print(f"imported {len(DCSS)} tiles into {os.path.normpath(OUT)}")


if __name__ == "__main__":
    main()
