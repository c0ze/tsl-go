# Monster Roster Batch 5 (13f) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Six more canonical 0.40 monsters (#13), drawn from `tsl-0.40/common/glyph.c`,
leaning into the bestiary's sci-fi/horror streak — which the existing level
names already support (the Laboratory, the Communications Hub). Free glyphs, no
collisions with the current roster.

## Monsters (names/glyphs verbatim from `tsl-0.40/common/glyph.c`)

| id | name | glyph | color | role | min_depth |
|----|------|-------|-------|------|-----------|
| `sentinel` | sentinel | `e` | cyan | ranged sentry (ranged 5) | 2 |
| `technician` | technician | `t` | normal | hostile lab tech | 2 |
| `burning_skull` | burning skull | `q` | red | fast floating skull (no corpse) | 3 |
| `gaoler` | gaoler | `G` | normal | jailer brute | 3 |
| `chrome_angel` | chrome angel | `A` | cyan | construct tank (no corpse) | 3 |
| `nameless_horror` | nameless horror | `H` | magenta | deep horror (no corpse) | 3 |

Stats follow the role conventions (fast/low-HP vs. slow/high-HP); the sentinel
reuses the 13c ranged AI + line-of-sight. Corpses reuse the generic
`corpse`/`carcass` for the humanoids; constructs, undead, and horrors leave none.

## Task 1: data + spawns + golden + ship
**Files:** `data/monsters.toml`, `data/levels.toml`, `data/data_test.go`
- Test first: `TestEmbeddedRosterBatch5` asserts the six load with the right
  glyphs, that the sentinel is ranged, and that **every** one appears in a spawn
  table (the strengthened seen-map assertion).
- Add the six `[monster.*]` blocks; wire them into mid/deep spawn tables (sci-fi
  foes in the Lab/Comm Hub, horrors in the deep floors); golden test green.
- Commit: `feat(content): monster roster batch 5 (sentinel, technician, …)`
- Full gate (`go test ./...`, `gofmt`); push, PR, CodeRabbit loop → merge.
  Advances #13.

## Done when
Six more faithful foes — sci-fi sentries and lab techs up top, constructs and
nameless horrors in the deep — prowl the dungeon, and the suite is green through
CodeRabbit.
