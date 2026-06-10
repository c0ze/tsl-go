# Potions of yuck & elixir (15r) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The last two portable entries of 0.40's `item_table_potion`
(`treasure.c` cases 10 and 12): the **potion of yuck** and the **elixir**.
With these, the potion table is complete except polymorph (waits on the
shapeshift subsystem). Advances #15.

## C grounding
- `potions.c treasure_p_yuck` (case 10): pure flavor — a 1d4 taste roll
  ("This tastes like ratman urine." 1/4, "...gnoblin blood." 2/4,
  "...liquified maggots." 1/4), identifies, no mechanical effect. (The
  gnoblin's "Yum!" needs monster quaffing — out of scope.)
- `potions.c treasure_elixir` (case 12): "Removes all status effects, even
  good ones" — expires poison/haste/slow/healing(regen)/stun/wound/blindness,
  zeroes levitation, then `change_altitude` (an airborne drinker lands!);
  "You feel perfectly normal." Effects we have that the C does *not* expire
  (confuse, fear, sleep) stay untouched.
- Names: "potion of yuck", "elixir" (plural "elixirs" — note: no "potion of"
  prefix in 0.40), both in the random-appearance pool.

## Design
- `Game.DispelLevitation()` — exported: if floating, drop the effect and run
  the landing (the C's `set_attr(levitate,0)` + `change_altitude`); behaviors
  can't reach the unexported `land`.
- Behavior `yuck`: roll the taste line via `g.RNG`, nothing else. Behavior
  `elixir`: `RemoveEffect` poison/haste/slow/regen/blind, then
  `DispelLevitation()`, return the C message.
- Data: `potion_yuck` (use `yuck`), `elixir` (kind potion, name "elixir",
  use `elixir`). Both spawn + join the appearance shuffle automatically.

## Task 1: DispelLevitation + behaviors (TDD)
**Files:** `internal/game/effects.go` + test, `internal/behaviors/behaviors.go`
+ test.
- Tests first: DispelLevitation over water plunges (landing runs);
  elixir strips poison+haste+blind but leaves fear; yuck logs a taste line
  and nothing else.
- Commit: `feat(game): DispelLevitation + yuck/elixir behaviors`

## Task 2: data + golden + ship
- Golden: both defs load; elixir is named "elixir" (no "potion of" prefix).
- Commit: `feat(content): potion of yuck + elixir (potion table complete sans polymorph)`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
One unidentified bottle tastes like liquified maggots and does nothing; the
other strips every effect you have — poison cured, haste wasted, and if you
were floating over a pool, enjoy the swim. Suite green.
