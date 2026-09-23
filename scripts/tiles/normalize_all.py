#!/usr/bin/env python3
"""Normalise every generated source image into assets/tiles/gen/.

  normalize_all.py SRC_DIR     # SRC_DIR/<name>.png for each name in sources.GEN

Missing sources are reported, not fatal, so a partial batch can be built.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from normalize import normalize, normalize_tile  # noqa: E402
from sources import GEN, GEN_ITEMS, GEN_TILE_CROP, GEN_TILES  # noqa: E402

OUT = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "tiles", "gen")


def main():
    src = sys.argv[1]
    os.makedirs(OUT, exist_ok=True)
    missing = []
    for name in GEN:
        p = os.path.join(src, name + ".png")
        if not os.path.exists(p):
            missing.append(name)
            continue
        if name in GEN_TILES:
            im = normalize_tile(p, box=GEN_TILE_CROP.get(name))
        elif name in GEN_ITEMS:
            im = normalize(p, fill=22, centre=True)
        else:
            im = normalize(p)
        im.save(os.path.join(OUT, name + ".png"))
    print(f"normalised {len(GEN) - len(missing)}/{len(GEN)}; missing: {' '.join(missing) or 'none'}")


if __name__ == "__main__":
    main()
