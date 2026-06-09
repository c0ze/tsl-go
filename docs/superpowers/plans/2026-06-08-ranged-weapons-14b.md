# Ranged Weapons 14b Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The player's answer to the imp — a wielded **ranged weapon** (a
shortbow) and a **fire (`f`)** action that aims the targeting cursor at a
creature and shoots it across the room, gated by range and line of sight. As a
thematic round-out, the **dragon and mummylich bosses gain ranged attacks** (fire
breath / dark bolts) using the same monster AI shipped in 13c.

## Design
- **`ItemDef.Ranged int`** — a weapon's firing range in tiles (0 = melee only).
  A ranged weapon still has melee stats (you can bash with a bow), but its real
  use is `f`.
- **`Game.FireWeapon(target)`** — requires a wielded weapon with `Ranged > 0`;
  rejects (no turn spent) when there's no ranged weapon, the target is out of
  range, or `lineOfSight` is blocked. On a valid shot it rolls the weapon's
  `Damage` against the creature at the target (`killCreature` on a kill) or
  reports a miss into empty space, then passes the turn. Reuses the existing
  `Prompter.Target` cursor and `lineOfSight`.
- **`ActFire` (`f`)** — `Run` checks for a wielded ranged weapon, prompts a
  target, and calls `FireWeapon`.
- **Bosses:** add `ranged` to the dragon and elder mummylich so they threaten
  from a distance (no engine change — the 13c AI already fires on `Ranged > 0`).

**Scope:** no ammunition (infinite shots) and no separate ranged-damage stat —
the weapon's `Damage` is the projectile. Throwing arbitrary items and quivers can
come later.

## Task 1: content — weapon Ranged (TDD)
**Files:** `internal/content/content.go`, `internal/content/content_test.go`
- Add `Ranged int` (`ranged`) to `ItemDef`; validate `>= 0`.
- Tests first: a ranged weapon loads; a negative `ranged` is rejected.
- Commit: `feat(content): weapon ranged stat`

## Task 2: game — FireWeapon (TDD)
**Files:** `internal/game/combat.go` (FireWeapon, WieldedRanged helper), tests
- `WieldedRanged() bool` (wielded weapon with `Ranged > 0`). `FireWeapon(target)`
  with the range/LOS guards above; valid shot damages + passes a turn.
- Tests first: fire kills a creature in range with LOS + spends a charge-free
  turn (monsters act); out of range → message, no damage, no turn; blocked LOS →
  no shot; no ranged weapon wielded → message.
- Commit: `feat(game): fire a wielded ranged weapon`

## Task 3: ui — fire verb (TDD)
**Files:** `internal/ui/ui.go`, `internal/ui/ui_test.go`,
`internal/ui/tcell/screen.go`, `internal/ui/tcell/screen_test.go`
- `ActFire` in the enum; `Run` ActFire case (wielded ranged weapon → `Target` →
  `FireWeapon`, else "you have no ranged weapon"). tcell `f` → `ActFire`.
- Tests first: Run fires at a creature via a fake Target; tcell `f` → `ActFire`.
- Commit: `feat(ui): fire (f) a ranged weapon`

## Task 4: data — shortbow + boss ranged + golden + ship
**Files:** `data/items.toml`, `data/monsters.toml`, `data/data_test.go`
- `shortbow` (kind weapon, glyph `}`, `ranged = 6`, modest melee). Give `dragon`
  and `elder_mummylich` a `ranged`. Golden tests: shortbow `ranged > 0`; bosses
  `ranged > 0`.
- Commit: `feat(data): shortbow + ranged bosses (fire breath)`
- Then full gate; push, PR, CodeRabbit loop → merge → pull master. Advances #14.

## Done when
Wield a shortbow, press `f`, aim at a monster down the corridor, and drop it —
while the dragon roasts you from across its lair. Suite green.
