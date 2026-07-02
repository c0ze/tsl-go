# tsl-go

[![CI](https://github.com/c0ze/tsl-go/actions/workflows/ci.yml/badge.svg)](https://github.com/c0ze/tsl-go/actions/workflows/ci.yml)

A faithful Go port of **The Slimy Lichmummy 0.40**, Ulf Åström's terminal
roguelike. The goal is full feature parity with the original — it should play
exactly like 0.40 — ported mechanic by mechanic from the C source, which is
preserved untouched in [`tsl-0.40/`](tsl-0.40/) as the reference. The
presentation gets a modern coat of paint; the plain original look is kept at
[`/original`](https://tsl.coze.org/original).

Descend the dungeon graph, identify potions the hard way, cross the Drowned
City's pools by levitation or fins, learn spells from books that sometimes
bite back, and take the ascension altar — or be remembered by `morgue.txt`.

## Status

The 0.40 parity roadmap is **complete**: the turn-energy scheduler and every
speed modifier, the full potion (13/13), scroll (10/10), and spellbook tables,
spell memorization, ~45 monsters including all five uniques the original can
actually spawn, hidden traps, water/swimming/levitation, lava, breath
weapons, ammunition, allies, polymorph, and save/resume. Content the original
defines but can never reach (three unspawnable uniques, the commented-out
manual of camouflage) is deliberately absent, catalogued for a possible 0.41.

## Play

**In the browser: <https://tsl.coze.org>** — the same engine compiled to
WebAssembly, saves in localStorage. The web build adds what a terminal can't:
per-level background music, synthesized sound effects, and a **Tiles** button
that swaps the ASCII map for a graphic tileset (torch-lit, with a camera that
follows you). The plain 0.40.1 build — before the visuals and audio — is kept
at **<https://tsl.coze.org/original>**. Or play natively:

```sh
go run ./cmd/tsl          # or grab a release binary, no install needed
```

The terminal renders in truecolour: box-drawing walls, deep-water and lava
glyphs, and a graded field of view that pools warm torchlight around you and
fades the rest to remembered shadow. Standalone executables for Linux, macOS,
and Windows are attached to every
[release](https://github.com/c0ze/tsl-go/releases).

### Keys

| Key | Action | Key | Action |
|---|---|---|---|
| h j k l y u b n | move (vi keys) | `g` | pick up |
| `i` | inventory / use | `e` | eat |
| `r` | read scroll or study a book | `z` | zap wand |
| `f` | fire bow | `c` | cast a learned spell |
| `t` | talk | `>` | take stairs |
| `S` | save and quit | `q` | quit |

Saving writes `~/.tsl-save.json` and exits; the next launch resumes and
deletes the savefile — saving is a free action, and there is no save-scumming.

In the browser, **m** mutes the music (a volume slider sits beside it) and the
**Tiles** button switches between the ASCII and graphic renderers.

## Develop

```sh
go test ./...   # the whole suite; every mechanic is grounded in the C source
go vet ./...
```

- `cmd/tsl/` — terminal front-end (tcell); `cmd/tsl-wasm/` + `web/` — the
  browser build (WebAssembly), with the music/SFX controllers and the canvas
  tile renderer in `web/`.
- `internal/game/` — the I/O-free engine: scheduler, combat, effects,
  hazards, save/load.
- `internal/behaviors/` — named item/spell effects, injected by `cmd` so the
  engine stays cycle-free.
- `internal/{content,gen,fov,rng,ui}/` — TOML content loading (fail-fast
  validation), level generation, field of view, the original's Mersenne
  Twister, and the rendering/input boundary.
- `data/` — all game content as TOML: tiles, monsters, items, the dungeon
  graph.
- `docs/superpowers/plans/` — one dated plan per shipped increment, each
  citing the C functions it ports.
- `tsl-0.40/` — the original 0.40 source distribution, untouched. When in
  doubt, the C is the spec: `tsl-0.40/common/*.c`.

CI runs gofmt, vet, and the full test suite on every push; pushing a `v*`
tag builds and publishes the cross-platform binaries.

## Versioning

Two release lines, so the faithful port stays findable forever:

- **v0.40.x — the faithful line.** Plays exactly like the original 0.40 as
  the C source defines it: identical mechanics, plain presentation, no
  graphics, no audio. Bug fixes only. v0.40.1 is this line's current release
  and the build served at [/original](https://tsl.coze.org/original).
- **v0.50+ — the enhanced line.** Everything beyond the original ships
  here: the truecolour torch-lit rendering, the graphic tile mode, music and
  sound effects, HUD polish, and any new mechanics (v0.50.0, v0.51.0, …).

Before tagging, ask which line the changes belong to; an enhancement must
never ship under v0.40.x.

## License

The port derives from The Slimy Lichmummy by Ulf Åström; see
[`tsl-0.40/LICENSE.TXT`](tsl-0.40/LICENSE.TXT) (and
[`tsl-0.40/COPYING_MT.TXT`](tsl-0.40/COPYING_MT.TXT) for the Mersenne
Twister). The Go port is offered under the same terms.
