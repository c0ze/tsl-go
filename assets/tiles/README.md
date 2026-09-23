# Web tile art

The sources of the browser build's graphic tiles (the **Tiles** button). They
are packed into `web/sprites.png` + `web/sprites.js` by
`scripts/tiles/build.py`; `scripts/tiles/sources.py` says where each sprite
comes from and which walls and floors each level uses.

| Directory | What | Licence |
|---|---|---|
| `dcss/` | Tiles from the **Dungeon Crawl Stone Soup** tileset (supplemental release), a handful hue-shifted (the red dragon, the green slime, the cave snake, the crystal sword). Copied in by `scripts/tiles/import_dcss.py`. | CC0 |
| `gen/` | Sprites made for this project where DCSS has no match — generated with Google Antigravity (`agy`, given DCSS tiles as style references) and the local Qwen-Image model, then cut down to 32x32 by `scripts/tiles/normalize.py`. | Project art |
| `anim/` | Aseprite sources for the animated tiles: palette-cycled water and lava (`scripts/tiles/liquid_frames.py` + `assemble.lua`) and the player's torch. | Project art |

## Credits

The Dungeon Crawl Stone Soup tiles were released to the public domain (CC0)
by their artists — thank you to the Crawl and Crawl Stone Soup teams and
everyone listed in the tileset's `README.txt`, including Eino Keskitalo,
David Lawrence Ramsey, Enne Walker, Poor_Yurik, and Stefan O'Rear, and to
the original RLTiles. The pack: <https://opengameart.org/content/dungeon-crawl-32x32-tiles-supplemental>
(also <https://github.com/crawl/tiles>).

## Rebuilding

```sh
python3 scripts/tiles/import_dcss.py "/path/to/Dungeon Crawl Stone Soup Supplemental"  # only when sources.DCSS changes
python3 scripts/tiles/normalize_all.py /path/to/generated/pngs                         # only for new generated art
python3 scripts/tiles/build.py                                                         # always: repack the atlas
```

`build.py` needs Python 3 with Pillow and the Aseprite CLI (`aseprite`) on
the PATH for the animations.
