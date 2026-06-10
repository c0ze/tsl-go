# Potions of speed & slowing (16b) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The content half of the turn-energy arc: **potion of speed** and
**potion of slowing**, both in 0.40's `item_table_potion` spawn table
(`treasure.c` cases 4–5). The 16a scheduler already gives `haste`/`slow` on the
player real teeth (free actions / double monster ticks); these potions are the
faithful way to acquire them. Advances #15.

## C grounding
- `treasure.c:1012`: "potion of speed", description "It speeds you up, duh.";
  `treasure.c:992`: "potion of slowing", "Slows a creature down." Both spawn
  from `item_table_potion` (`treasure.c:2294`), so they join floor loot and the
  random-appearance shuffle like every potion.
- `potions.c treasure_p_speed`: expire `effect_slow`, prolong `effect_haste`
  for `HASTE_LENGTH` (20); message "You move faster!".
- `potions.c treasure_p_slowing`: expire `effect_haste`, prolong `effect_slow`
  for `SLOW_LENGTH` (20); message "You feel very sluggish.".

## Design
- `Game.RemoveEffect(kind)` — the engine hook for the C's `effect_expire`:
  drops an active player effect by kind (no-op when absent).
- Behaviors `haste` and `slowness` (registered): cancel the opposite effect,
  apply theirs for `Power` turns, return the C message.
- Data: `potion_speed` (use `haste`, power 20 = `HASTE_LENGTH`) and
  `potion_slowing` (use `slowness`, power 20 = `SLOW_LENGTH`); spawnable
  (no `nospawn`), so they enter loot and the appearance pool automatically.

## Task 1: RemoveEffect + behaviors (TDD)
**Files:** `internal/game/effects.go` + test, `internal/behaviors/behaviors.go`
+ test.
- Tests first: `RemoveEffect` drops only the named effect; `haste` behavior
  applies haste *and* cancels an active slow; `slowness` applies slow and
  cancels an active haste.
- Commit: `feat(game): RemoveEffect + haste/slowness behaviors`

## Task 2: data + golden + ship
**Files:** `data/items.toml`, `data/data_test.go`.
- Tests first: both defs load with the right kind/use/power.
- Commit: `feat(content): potions of speed and slowing`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Quaffing an unidentified potion can now grant 20 turns of haste (periodic free
actions) or sandbag you for 20 turns while the pack closes in twice per step —
and identifies the type, like 0.40. Suite green.
