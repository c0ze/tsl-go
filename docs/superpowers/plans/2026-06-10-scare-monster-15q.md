# Scroll of Scare Monster (15q) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** A **scroll of scare monster** — a new `fear` status effect with **flee**
AI (the inverse of pursuit): a frightened creature runs *away* from the player
and won't attack. Delivered as a self-centred AoE (read the scroll → every nearby
creature flees), reusing the `affectNearby` helper from #50. It pairs with the
confuse wand (#52) as a second crowd-control tool and rounds out the scroll kit.
Advances #15.

## Design
- **`fear` effect:** HUD label ("Afraid") + verb ("frightens").
- **Flee AI** (`monsterAct`): a branch after the confuse check — if the creature
  `HasEffect("fear")`, call a new `stepAway(m, g.Player)` and return (so it never
  attacks/pursues). `stepAway` steps one tile directly away from the player using
  the existing `signOf`; if that tile is blocked (wall, another creature, or the
  player), the cornered creature holds.
- **AoE delivery:** an exported `ScareNearby(radius, turns) int` wrapping
  `affectNearby(radius, "fear", turns)` (mirrors `FlashBlind`/`PoisonNearby`); a
  `scare` behaviour calls it; `scroll_scare_monster` (kind scroll, use `scare`)
  is read via the existing read verb.

## Task 1: fear effect + flee AI (TDD)
**Files:** `internal/game/effects.go`, `internal/game/combat.go`,
`internal/game/spell.go`, `internal/game/*_test.go`
- Tests first: a feared creature adjacent to the player **flees** — it increases
  its distance and deals no damage; `ScareNearby` applies fear within radius
  (sparing a far creature).
- Implement the label/verb, the `monsterAct` fear branch + `stepAway`, and
  `ScareNearby` + `ScareRadius`.
- Commit: `feat(game): fear status effect (flee AI) + ScareNearby`

## Task 2: behaviour + data + golden + ship
**Files:** `internal/behaviors/behaviors.go`, `internal/behaviors/behaviors_test.go`,
`data/items.toml`, `data/data_test.go`
- Test first: the `scare` behaviour frightens nearby creatures; the embedded
  `scroll_scare_monster` loads with use `scare`.
- Implement `scare`, register it; add the scroll; golden test.
- Commit: `feat(content): scroll of scare monster (AoE fear)`
- Full gate; push, PR, CodeRabbit loop → merge.

## Done when
Read a scroll of scare monster and the pack around you turns and flees instead of
closing in, buying you room to escape or pick them off. Suite green.
