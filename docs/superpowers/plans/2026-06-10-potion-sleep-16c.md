# Potion of sleep (16c) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The **potion of sleep** (0.40 `item_table_potion` case 8) and the
`sleep` status effect — the third class unlocked by the 16a scheduler: a
sleeping creature simply doesn't get its turns. Advances #15.

## C grounding
- `potions.c treasure_p_sleep` → `tranquilize()` (`sleep.c`): apply
  `effect_sleep` for `SLEEP_DURATION` (25, `rules.h:126`); "You fall asleep!";
  always identifies for the player.
- `game.c:178-191`: when a sleeping creature's turn comes up, the turn is
  skipped (ttl decrements); at the end it wakes — "You wake up!" /
  "wakes up.".
- `combat.c:479`: being attacked wakes the defender (`creature_sleep(d, false)`).
- While the player sleeps, the C main loop keeps running without prompting —
  the world advances around the sleeper; `pass_time_on_effects`/`energy()`
  still run each of the sleeper's turns (poison ticks, EP regens).

## Design
- Effect kind `sleep` (HUD "Asleep").
- **Player:** split `advanceWorld()` into `passTurn()` (the existing body) plus
  a wrapper loop: `passTurn(); for HasEffect("sleep") && !Dead { passTurn() }`.
  Any sleep source — this potion, future traps — immediately runs the slept
  turns with no player action, exactly the C pacing. TTL burns via
  `tickEffects` (≤ Power iterations); on natural expiry tickEffects logs
  "You wake up!".
- **Wake on damage:** `HurtPlayer` (melee + ranged both funnel through it):
  if the player survives and is asleep, `RemoveEffect("sleep")` + "You wake
  up!" — exits the sleep loop early.
- **Monsters:** `monsterAct` holds while `sleep` is active (like blind's hold);
  `playerAttacks` wakes a sleeping target via a new `Creature.RemoveEffect`.
- **Behavior `tranquilize`** (the C's name): `AddEffect("sleep", Power)`,
  "You fall asleep!". Data: `potion_sleep`, use `tranquilize`, power 25.
  PlayerUse already identifies on quaff (C: always identifies).
- Out of scope: monster-targeted sleep (wands/throwing), spawn-asleep AI,
  `attr_p_sleep` immunity — none needed for the potion.

## Task 1: sleep effect + scheduler loop + wake rules (TDD)
**Files:** `internal/game/{effects,combat,creature}.go` + tests.
- Tests first: one player action while asleep(3) passes 3 extra world ticks
  (rat closes 4 tiles total) and ends with "You wake up!"; a monster's hit
  wakes the sleeping player early; a sleeping monster doesn't attack from
  adjacency; hitting a sleeping monster removes its sleep.
- Commit: `feat(game): sleep status effect (turn-skipping + wake-on-damage)`

## Task 2: behavior + data + ship
**Files:** `internal/behaviors/behaviors.go` + test, `data/items.toml`,
`data/data_test.go`.
- Tests first: `tranquilize` applies sleep for Power turns with the C message;
  `potion_sleep` loads (potion / tranquilize / 25).
- Commit: `feat(content): potion of sleep`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Quaffing the wrong unidentified potion knocks you out for 25 turns while the
dungeon keeps moving — unless something hits you awake first. Sleeping monsters
hold until struck. Suite green.
