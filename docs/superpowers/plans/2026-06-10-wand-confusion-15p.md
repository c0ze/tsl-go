# Wand of Confusion (15p) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** A **wand of confusion** — a new `confuse` status effect with its own AI
behaviour: a confused creature lurches in a random direction each turn and is too
disoriented to press an attack, breaking a dangerous foe's pursuit. It parallels
the existing `blind` mechanic and reuses the effect-wand path (like the wands of
venom/slowing), so the wand itself needs no new zap code. Advances #15.

## Design
- **`confuse` effect:** add HUD label ("Confused") and zap verb ("confuses") to
  `effectLabels`/`effectVerbs`.
- **AI branch** (`monsterAct`): a top-of-function check — if the creature
  `HasEffect("confuse")`, call a new `stepRandom(m)` and return, so it never
  reaches the attack/track/pursue logic. `stepRandom` moves one tile in a random
  direction via `g.RNG`; it won't lurch onto the player (so a confused creature
  can't attack), another creature, or a wall — those just stumble in place.
- **Wand:** `wand_confusion` (kind wand, `effect = "confuse"`, `effect_turns`).
  The existing `ZapWand` effect path applies it.

## Task 1: confuse effect + wandering AI (TDD)
**Files:** `internal/game/effects.go`, `internal/game/combat.go`,
`internal/game/combat_test.go`
- Tests first: a confused creature adjacent to the player does **not** damage it
  over a turn (it can't attack), while a non-confused control creature does; the
  confused creature still ticks the effect down and resumes attacking once it
  expires.
- Implement the label/verb entries, the `monsterAct` confuse branch, and
  `stepRandom`.
- Commit: `feat(game): confuse status effect (wandering AI)`

## Task 2: wand data + golden + ship
**Files:** `data/items.toml`, `data/data_test.go`
- Test first: the embedded `wand_confusion` loads with `effect = "confuse"`.
- Add the wand; golden test green.
- Commit: `feat(content): wand of confusion`
- Full gate (`go test ./...`, `gofmt`); push, PR, CodeRabbit loop → merge.

## Done when
Zap a charging foe with the wand of confusion and it lurches harmlessly around
the room instead of attacking, until the confusion wears off. Suite green.
