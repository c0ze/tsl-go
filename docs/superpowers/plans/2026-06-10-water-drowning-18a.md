# Water & drowning (18a) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** First PR of the swimming arc (`swimming.c`): **water pools** as
terrain plus the **drowning** rules for the player. In 0.40 water is
*impassable on foot* — `creature.c move_creature` only admits `free_swim`/
`permaswim` creatures, floaters, the **blinded** (a stumble), or forced pushes —
so this PR ships the terrain, the blind-stumble entry, and the swim-fatigue/
drowning clock. Later PRs: potion of levitation (float over water/lava, the
player's real way in; 0.40 potion table case 7), then the Lurker pool
encounter. Advances #18.

## C grounding
- `tiles.c:37`: water is `walkable=false`, `opaque=false`; glyph `_` plain
  (`glyph.c:56 set_glyph(gent_water,'_',A_NORMAL)`).
- `creature.c move_creature`: a blinded creature may blunder into water; a
  sighted walker simply can't step in. Stepping from water back onto floor is
  a normal walk.
- `swimming.c swim()` (run right after each creature's turn, `game.c`):
  on water `swim_fatigue++`; when `fatigue > attr_swimming` (player base 0)
  take 1 damage; on dry land fatigue resets to 0. Player death: "You drown..."
  (morgue cause "drowned").
- Pools: `level->water` pools per level (`places.c`: the Dungeon `rnd%2`,
  worm-cave levels `3+rnd%3`, the Lurker level `5+rnd%5`), each an
  `area_cloud` blob of `5+rnd%20` tiles carved out of floor (`content.c:250`).
- Out of scope here: monster drowning/free-swim (no monster can enter water
  yet — pursuit only walks passable tiles), levitation, pushes, item
  destruction in water (nothing can drop there yet), lava.

## Design
- `TileDef.Water bool` (`water = true` in tiles.toml on a new `water` tile,
  passable=false so existing movement/AI treat it as a wall by default).
- **Blind stumble:** `PlayerStep` — when the destination tile is water and the
  player is blind, enter it (costs the turn); sighted bumps stay no-ops.
- **Swim clock:** `Game.swimFatigue int`. In `passTurn` (the C runs `swim()`
  per creature-turn): standing on water → `swimFatigue++`, and past the
  player's swimming skill (0, attr-less for now) lose 1 HP — death is
  `DeathCause` "drowned" with "You drown...". On land the counter resets.
- **Pools in gen:** per-level pool count range in levels.toml (dungeon 0–1
  like the C Dungeon; deeper levels 3–5), carved as random blobs of 5–24
  floor tiles turned to water.

## Task 1: water tile + stumble + drowning (TDD)
**Files:** `data/tiles.toml`, `internal/content` (Water flag),
`internal/game/{game,combat}.go`, new `internal/game/swim_test.go`.
- Tests first: sighted step into water is a no-turn bump; blind step enters;
  each turn standing in water costs 1 HP; wading out resets the clock (no
  damage after re-land); drowning at 0 HP sets cause "drowned" and logs
  "You drown...".
- Commit: `feat(game): water tiles, blind stumble-in, swim fatigue + drowning`

## Task 2: pools in level gen + ship
**Files:** `data/levels.toml`, `internal/content` (pool range),
`internal/gen` + tests.
- Tests first: a level configured with pools generates water tiles in a
  connected blob within the configured size band; a zero-pool level has none.
- Commit: `feat(gen): water pools carved per level (C add_pools)`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Deep levels grow dark pools of `_` that block your path; blunder in blind and
each turn under water bleeds 1 HP until you wade out — or drown. Suite green.
