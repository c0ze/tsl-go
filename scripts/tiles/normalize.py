#!/usr/bin/env python3
"""Turn a large generated sprite into a 32x32 tile that sits with the DCSS set.

  normalize.py in.png out.png [--fill 30] [--item] [--tile]

A cut-out (a monster) is cropped to its opaque subject, scaled (premultiplied
area filter) into a `fill`-pixel box, given a 1-bit alpha, a touch more
contrast, and the 1px near-black outline DCSS figures carry, then anchored
bottom-centre like the stock monster tiles. --item makes a smaller, centred
cut-out like the DCSS item icons; --tile a full-bleed terrain tile (doors,
stairs, trap plates).
"""
import argparse

from PIL import Image, ImageEnhance, ImageFilter


def knock_out_background(im, tol=48):
    """Make a solid backdrop transparent. Some generations come back on an
    opaque (usually white) field instead of alpha: when the border is one
    colour, flood-fill it away from every edge pixel so enclosed areas of
    the same colour inside the figure survive."""
    w, h = im.size
    px = im.load()
    border = [px[x, y] for x in range(w) for y in (0, h - 1)] + [px[x, y] for y in range(h) for x in (0, w - 1)]
    opaque = [p for p in border if p[3] > 200]
    if len(opaque) < 0.9 * len(border):
        return im  # already cut out
    ref = tuple(sorted(c[i] for c in opaque)[len(opaque) // 2] for i in range(3))

    def near(p):
        return p[3] > 0 and all(abs(p[i] - ref[i]) <= tol for i in range(3))

    seen = bytearray(w * h)
    stack = [(x, y) for x in range(w) for y in (0, h - 1)] + [(x, y) for y in range(h) for x in (0, w - 1)]
    while stack:
        x, y = stack.pop()
        if x < 0 or y < 0 or x >= w or y >= h or seen[y * w + x]:
            continue
        seen[y * w + x] = 1
        if not near(px[x, y]):
            continue
        px[x, y] = (0, 0, 0, 0)
        stack += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    return im


def normalize(src, size=32, fill=30, centre=False):
    im = knock_out_background(Image.open(src).convert("RGBA"))
    alpha = im.getchannel("A").point(lambda v: 255 if v > 40 else 0)
    im = im.crop(alpha.getbbox())
    w, h = im.size
    scale = (fill - 2) / max(w, h)  # leave room for the outline
    tw, th = max(1, round(w * scale)), max(1, round(h * scale))

    # Premultiply so transparent pixels don't bleed grey into the edges.
    r, g, b, a = im.split()
    pre = Image.merge("RGB", [Image.composite(c, Image.new("L", im.size, 0), a) for c in (r, g, b)])
    pre = pre.resize((tw, th), Image.Resampling.BOX)
    a = a.resize((tw, th), Image.Resampling.BOX)
    px, pa = pre.load(), a.load()
    out = Image.new("RGBA", (tw, th))
    po = out.load()
    for y in range(th):
        for x in range(tw):
            av = pa[x, y]
            if av < 110:
                continue
            cr, cg, cb = px[x, y]
            k = 255 / av
            po[x, y] = (min(255, int(cr * k)), min(255, int(cg * k)), min(255, int(cb * k)), 255)

    rgb = out.convert("RGB")
    rgb = ImageEnhance.Contrast(rgb).enhance(1.15)
    rgb = ImageEnhance.Color(rgb).enhance(1.1)
    out = Image.merge("RGBA", (*rgb.split(), out.getchannel("A")))

    tile = Image.new("RGBA", (size, size))
    ox = (size - tw) // 2
    # Monsters stand on the bottom row (one pixel up for the outline); items
    # sit in the middle of the cell.
    oy = (size - th) // 2 if centre else size - 1 - th
    tile.paste(out, (ox, oy), out)

    # 1px outline: any transparent pixel touching the subject turns near-black.
    mask = tile.getchannel("A").point(lambda v: 255 if v else 0)
    grown = mask.filter(ImageFilter.MaxFilter(3))
    tp, gp, mp = tile.load(), grown.load(), mask.load()
    for y in range(size):
        for x in range(size):
            if gp[x, y] and not mp[x, y]:
                tp[x, y] = (12, 10, 9, 255)
    return tile


def normalize_tile(src, size=32):
    """A full-bleed terrain tile: square centre crop, area-resized, opaque."""
    im = Image.open(src).convert("RGB")
    w, h = im.size
    s = min(w, h)
    im = im.crop(((w - s) // 2, (h - s) // 2, (w - s) // 2 + s, (h - s) // 2 + s))
    im = im.resize((size, size), Image.Resampling.BOX)
    im = ImageEnhance.Contrast(im).enhance(1.1)
    return im.convert("RGBA")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--size", type=int, default=32)
    ap.add_argument("--fill", type=int, default=30)
    ap.add_argument("--item", action="store_true", help="smaller centred cut-out (an item icon)")
    ap.add_argument("--tile", action="store_true", help="full-bleed terrain tile, not a cut-out")
    args = ap.parse_args()
    if args.tile:
        normalize_tile(args.src, args.size).save(args.dst)
    elif args.item:
        normalize(args.src, args.size, min(args.fill, 22), centre=True).save(args.dst)
    else:
        normalize(args.src, args.size, args.fill).save(args.dst)


if __name__ == "__main__":
    main()
