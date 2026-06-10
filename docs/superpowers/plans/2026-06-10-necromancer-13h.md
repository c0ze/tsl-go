# The Necromancer (13h) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The second unique: **the Necromancer**, boss of the Catacombs
(`places.c:130 temp->boss = encounter_necromancer`). Advances #13 (uniques).
Data-only — every mechanic he needs shipped in earlier increments.

## C grounding
- `unique.c:78`: HP 30, **speed 10** (glacial — roughly one action per ten
  player turns under the energy scheduler, a stationary-turret flavor the C
  shares), EP 200 with force bolt / frost ray / fireball / bone crush —
  approximated as a strong ranged caster (our casters are single-target
  bolts); `virtual_cold_touch` melee — mapped to the shipped on-hit effect
  machinery as a chilling touch (slow), the same mapping the wand of slowing
  uses.
- Glyph `N` (`glyph.c:109`, free in our roster), bestiary "NECROMANCER, THE (N)".
- His two random scrolls at build are rolled-and-discarded in the C
  (`del_item(attach_item_to_creature(...))`) — nothing to port.
- The King of Worms was considered and deferred: he's an interaction NPC
  ("the password that you want", `actions.c:876`) and needs the talk verb /
  quest subsystem, not a stat block.

## Design
`[monster.necromancer]`: N, normal color (C A_NORMAL), hp 30, attack 9,
dodge 2, damage "2d6", speed 10, ranged 7, effect "slow" 3 (cold touch),
min_depth 99 (boss-only). Catacombs gets `boss = "necromancer"`.

## Task: def + boss wiring + golden (TDD)
**Files:** `data/monsters.toml`, `data/levels.toml`, `data/data_test.go`.
- Golden test first (red): def stats/flags + catacombs boss wiring.
- Fill TOML (green); full gate; push, PR, CodeRabbit loop → merge.
- Commit: `feat(content): the Necromancer — boss of the Catacombs`

## Done when
An `N` waits deep in the Catacombs: almost motionless, hurling bolts from
across the room, its rare touch draining the speed from your limbs. Suite
green.
