# Monster Roster Batch 4 (13e) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Six more canonical 0.40 monsters (#13), drawn straight from the
bestiary glyphs in `tsl-0.40/common/glyph.c` — broadening the mid-to-deep tiers. The
0.40 bestiary is much larger than what's ported so far; this batch adds beasts
with free glyphs (no collisions with the current roster).

## Monsters (names/glyphs verbatim from `tsl-0.40/common/glyph.c`)

| id | name | glyph | color | role | min_depth |
|----|------|-------|-------|------|-----------|
| `hellhound` | hellhound | `h` | red | fast pack hunter | 2 |
| `frostling` | frostling | `f` | cyan | nimble skirmisher (no corpse) | 2 |
| `goatman` | goatman | `p` | brown | sturdy melee | 2 |
| `tentacle` | tentacle | `l` | green | slow grabber (no corpse) | 3 |
| `gloom_lord` | gloom lord | `K` | magenta | ranged caster (ranged 5) | 3 |
| `giant_slimy_toad` | giant slimy toad | `Y` | green | deep tank | 3 |

Stats follow the existing role conventions (fast/low-HP skirmishers vs.
slow/high-HP tanks); the ranged gloom lord reuses the 13c ranged AI + LOS.
Corpses reuse the generic `carcass` where edible; elementals/oozes leave none.

## Task 1: data + spawns + golden + ship
**Files:** `data/monsters.toml`, `data/levels.toml`, `data/data_test.go`
- Test first: `TestEmbeddedRosterBatch4` asserts the six load with the right
  glyphs and that at least one appears in a spawn table (mirrors batch 3).
- Add the six `[monster.*]` blocks; wire them into mid/deep spawn tables at low
  weights (depth enforced by which levels list them); golden test green.
- Commit: `feat(content): monster roster batch 4 (hellhound, frostling, …)`
- Full gate (`go test ./...`, `gofmt`); push, PR, CodeRabbit loop → merge.
  Advances #13.

## Done when
Six more faithful foes prowl the mid-to-deep floors — including a second ranged
caster (the gloom lord) — and the suite is green through CodeRabbit.
