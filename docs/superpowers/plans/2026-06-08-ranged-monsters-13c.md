# Ranged Monsters 13c Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The first AI variety beyond melee-chase: a monster that **blasts you
from a distance**. When it has a clear line of sight and you're within range
(but not adjacent), it fires a bolt instead of closing in — so you can be hurt
across a room and want to break line of sight or rush it. Ships an **imp**, a
fast ranged caster.

## Design
- **`MonsterDef.Ranged int`** — the attack range in tiles (0 = melee only). A
  ranged monster reuses its `Damage` spec for the bolt and `Attack` vs the
  player's dodge for the to-hit roll.
- **Line of sight** — `Game.lineOfSight(a, b)` walks a Bresenham line from a to
  b and returns false if any *intermediate* tile is opaque (`!Transparent`), so
  walls and closed doors block bolts (and an open door lets them through).
- **`monsterAct`** gains a branch: when not adjacent, if `Ranged > 0`, the target
  is within `Ranged`, and there's line of sight, fire (`rangedAttack`) and don't
  move; otherwise fall back to stepping toward the player.
- **Refactor:** extract `hurtPlayer(dmg, cause)` (the HP-drop + death block) so
  melee and ranged share one death path.

**Scope:** straight-line bolts at the player only; no friendly fire, no beam
that hits everything in the path, no separate ranged-damage stat. Faithful enough
for a caster; richer spell effects can layer on the status system later.

## Task 1: content — Ranged stat (TDD)
**Files:** `internal/content/content.go`, `internal/content/content_test.go`
- Add `Ranged int` (`ranged`) to `MonsterDef`; validate `>= 0`.
- Tests first: a ranged monster loads; a negative `ranged` is rejected.
- Commit: `feat(content): monster ranged attack stat`

## Task 2: game — line of sight + ranged AI (TDD)
**Files:** `internal/game/combat.go` (monsterAct, rangedAttack, hurtPlayer),
`internal/game/los.go` (lineOfSight), tests
- `lineOfSight` Bresenham, intermediate opacity check. `rangedAttack` mirrors
  melee mechanics with a bolt message via `hurtPlayer`. `monsterAct` ranged
  branch.
- Tests first: clear LOS true, wall-blocked LOS false; a ranged monster in range
  with LOS damages the player without moving adjacent; a wall between blocks the
  shot (it approaches instead); out-of-range it approaches.
- Commit: `feat(game): line-of-sight ranged monster AI`

## Task 3: data — the imp + golden + ship
**Files:** `data/monsters.toml`, `data/data_test.go`, level spawn tables
- `imp` (`i`, red, fast, `ranged = 5`, mid/deep tiers, no corpse). Add to a few
  spawn tables. Golden test: imp loads with `ranged > 0`.
- Commit: `feat(data): imp (ranged caster)`
- Then full gate; push, PR, CodeRabbit loop → merge → pull master. Advances #13.

## Done when
An imp pelts you with bolts across a lit room; duck behind a wall or a closed
door and the bolts stop. Suite green.
