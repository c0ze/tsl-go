# Consumable Breadth 15h Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Widen the consumable lists (#15) with four items that build on existing
mechanics and make the new identify system *matter*: a **potion of instant
healing** (good) and a **potion of pain** (bad — now an unknown potion can hurt
you), a **scroll of identify** (closes the identify loop), and a **scroll of
recharge** (refills a wand).

## Faithful reference (`common/potions.c`)
- `treasure_p_instant_healing`: `heal(creature, 10 + rnd() % 15)` → restore 10–24.
- `treasure_p_pain`: damages the creature.
- scrolls of identify / recharge: identify an item / restore wand charges.

(`treasure_p_energy`, polymorph, levitation, sleep, speed, slowing, blindness and
the spell-y scrolls want a player status/EP model and come in later increments.)

## Design
- **Engine helpers** (so behaviors, which act only through exported API, can do
  this): export `HurtPlayer(dmg, cause)` (today's unexported `hurtPlayer`); add
  `IdentifyItem(it)` (mark a type known, no message), `IsIdentified(it)`, and
  `UnidentifiedInventory()` (carried items still hidden).
- **Behaviors** (`internal/behaviors`):
  - `instant_healing`: `restoreHP(10 + RNG.Intn(15))`.
  - `pain`: `HurtPlayer(Power, "a potion of pain")` — can kill at low HP.
  - `identify`: reveal one random still-unidentified carried item (the scroll has
    already use-identified itself), reporting "Your <appearance> is a <name>.";
    "nothing new" when the pack is fully known.
  - `recharge`: add 3–5 charges to a random carried wand.
- Also commit the **morgue identify test** left out of #39 (test-only coverage
  for the already-merged morgue `DisplayName` change).

**Scope:** identify/recharge pick a random eligible item rather than prompting
(behaviors can't drive the Prompter); a chosen-target version is a later
refinement.

## Task 1: game — helpers (TDD)
**Files:** `internal/game/combat.go` (export HurtPlayer + callers),
`internal/game/identify.go` (IdentifyItem/IsIdentified/UnidentifiedInventory),
tests (+ restore morgue test)
- Tests first: HurtPlayer damages + resolves death; UnidentifiedInventory lists
  only hidden carried items and drops them once identified.
- Commit: `feat(game): export HurtPlayer + identify-item helpers`

## Task 2: behaviors — four effects (TDD)
**Files:** `internal/behaviors/behaviors.go`, `internal/behaviors/behaviors_test.go`
- Implement + register `instant_healing`, `pain`, `identify`, `recharge`.
- Tests first: instant healing restores HP; pain costs HP (and can kill); identify
  reveals an unidentified carried item; recharge adds wand charges.
- Commit: `feat(behaviors): instant healing, pain, identify, recharge`

## Task 3: data — four consumables + golden + ship
**Files:** `data/items.toml`, `data/data_test.go`
- `potion_instant_healing`, `potion_pain` (power = damage), `scroll_identify`,
  `scroll_recharge`. Golden test for all four.
- Commit: `feat(data): instant-healing/pain potions + identify/recharge scrolls`
- Full gate; push, PR, CodeRabbit loop → merge → pull master. Advances #15.

## Done when
Quaffing an unidentified potion might heal you to the brim or wrack you with pain;
a scroll of identify names a mystery item, and a scroll of recharge tops up a
wand. Suite green.
