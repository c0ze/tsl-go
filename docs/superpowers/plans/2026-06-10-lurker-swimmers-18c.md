# Swimming monsters & the Lurker (18c) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Swimming arc 3/3: monsters that swim, and the first unique — **the
Lurker** — coiled in a Drowned City pool ringed by tentacles. Advances #18 +
#13 (uniques).

## C grounding
- `creature.c move_creature`: `attr_free_swim` admits water; `attr_permaswim`
  refuses anything *but* water ("we'd rather not get out").
- `monster.c`: the merman (already in our roster) has `free_swim`; the
  tentacle has `free_swim` + `permaswim` (a tentacle out of water is stuck —
  it can flail but not move).
- `swimming.c swim()`: free-swimmers never drown.
- `unique.c:149` the Lurker: glyph `L` (`glyph.c:107`), HP 22, attack 95,
  dodge 0, speed 120, free_swim + permaswim + sleep-immune, unarmed
  `virtual_poison_fangs` (~2d6, damage_poison, "bites").
- `content.c encounter_lurker`: **force the spot to water** ("Reset the
  lurkers pool"), place the Lurker, then 8 tentacles at the nearest free
  spots.
- Out of scope: noxious breath for the Lurker (our ranged AI is single-target
  bolts; revisit with the breath/explode subsystem), sleep immunity (nothing
  can sleep a monster yet).

## Design
- `MonsterDef`: `swim` (may enter water), `permaswim` (water only), and
  on-hit `effect`/`effect_turns` (poison fangs; mirrors wand/tile fields,
  validated the same way). `monsterAttacks` applies the effect when the
  player survives the bite.
- Movement: one `creatureCanEnter(m, dst)` shared by
  stepToward/stepAway/stepRandom — permaswim → only water tiles; swim →
  passable or water; otherwise passable. (stepToward keeps its door-opening
  fallback for walkers.)
- Data: `lurker` (L, 22/95/0, speed 120, "2d6" + poison 5, nospawn-by-table:
  placed as a boss), merman `swim = true`, tentacle `swim` + `permaswim`.
- Gen: a permaswim boss gets its tile **forced to water** (the C's pool
  reset); new `retinue`/`retinue_count` on LevelDef spawns escorts at the
  nearest free tiles around the boss (water first). The Drowned City gets
  `boss = "lurker"`, retinue 8 tentacles.

## Task 1: swim flags + movement + poison fangs (TDD)
**Files:** `internal/content/content.go`, `internal/game/combat.go`,
`internal/game/swim_test.go` (extend).
- Tests first: a swim-flagged monster pursues the player straight through a
  pool (non-swimmer detours/stalls at the bank); a permaswim creature never
  steps onto floor; a fanged monster's hit poisons.
- Commit: `feat(game): swimming monsters (free_swim/permaswim) + on-hit effects`

## Task 2: the Lurker + retinue placement + ship
**Files:** `data/monsters.toml`, `data/levels.toml`, `data/data_test.go`,
`internal/gen/gen.go` + tests.
- Tests first: gen forces water under a permaswim boss and rings it with the
  retinue; golden data for the lurker def and Drowned City wiring.
- Commit: `feat(content): the Lurker — pool boss with tentacle retinue`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Mermen cut straight across the Drowned City's pools while you detour; one pool
holds something far worse — an `L` that never leaves the water, ringed by
tentacles, whose bite leaves poison in your veins. Suite green.
