# Potion of levitation (18b) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Swimming arc 2/3: the **potion of levitation** (0.40
`item_table_potion` case 7) — the player's legitimate way across the water of
18a. Floating crosses deep water and skips floor traps; the danger is the
clock: when it runs out you land on whatever is below. Advances #18 + #15.

## C grounding
- `potions.c treasure_p_levitation` → `levitate()`: float for `LEVITATE_TIME`
  (20, `rules.h:129`); "You soar into the air!" (`altitude.c:58`).
- `creature.c move_creature`: `is_floating` admits water (and lava) tiles.
- `swimming.c swim()`: early-returns for floaters — no fatigue while airborne
  (the counter freezes; it neither accrues nor resets).
- `traps.c activate_trap:325`: floaters skip traps — except `trap_win`, so the
  ascension altar still works mid-float.
- `game.c decrement_levitation` → `altitude.c change_altitude` on expiry:
  land in water → "You plunge into water!" (the swim clock takes over);
  land on a trap → it springs; otherwise "You land on the ground.".

## Design
- Effect `levitate` (HUD "Floating"); behavior `levitate`:
  `AddEffect("levitate", Power)` + the C soar message. Data:
  `potion_levitation`, use `levitate`, power 20.
- `PlayerStep`: the water-entry branch admits `blind` **or** `levitate`; the
  trap branch (`tile.Effect`) is skipped while floating; `Win` still fires.
- `swimCheck`: early return while floating (fatigue frozen, like the C).
- Landing (`tickEffects`, on levitate expiry — like sleep's wake message):
  on water → "You plunge into water!"; on an effect tile → "You land on the
  ground." + the trap's effect; else the ground message.

## Task 1: levitate effect + movement/trap/landing rules (TDD)
**Files:** `internal/game/{combat,effects}.go`, `internal/game/swim_test.go`
(extend).
- Tests first: a floating player crosses water unharmed (no HP loss, full
  pass); floating over a dart trap doesn't poison; expiry over water plunges
  (message + drowning clock resumes next turn); expiry over a trap springs it;
  expiry on floor logs the ground message.
- Commit: `feat(game): levitation — float over water/traps, landing rules`

## Task 2: behavior + data + ship
**Files:** `internal/behaviors/behaviors.go` + test, `data/items.toml`,
`data/data_test.go`.
- Tests first: `levitate` behavior + golden data (potion/levitate/20).
- Commit: `feat(content): potion of levitation`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Quaff, soar, drift over the Drowned City's pools and the traps between — and
time the crossing right, because landing in deep water starts the drowning
clock. Suite green.
