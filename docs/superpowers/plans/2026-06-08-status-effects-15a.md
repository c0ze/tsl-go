# Status Effects 15a Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** The status-effect system — the keystone for #15 (potions/scrolls) and #18 (traps/poison). First slice: the framework + **poison** and **regeneration**, plus the red **mushroom** (heal + poison, completing the #12 eat mechanic) and a **regeneration potion**.

## Design
- `Effect{Kind string, Turns int}` on the player: `Game.Effects []Effect`. (Player-only for now; monster-targeted effects arrive with wands/targeting in a later increment.)
- `AddEffect(kind, turns)` — adds, or refreshes to the longer duration if already active.
- `tickEffects()` — runs once per player action (hooked at the top of `monstersAct`): `poison` does −1 HP/turn (can kill, cause "poison"); `regen` does +1 HP/turn (clamped to max); each effect's `Turns` decrements and expires at 0. If poison kills, the world doesn't act further that turn.
- `EffectsSummary()` → e.g. "Poisoned, Regenerating" for the HUD.
- Consumables apply effects via behaviors: `regenerate` (potion) and `eat_mushroom` (food: heal + poison).

**Scope note:** later 15 increments add blindness (FOV) + sleep + the bad-potion line; wands + **interactive targeting**; scrolls + spellbooks. Speed/haste (scheduler-coupled) is its own slice.

## Task 1: engine — effects framework
**Files:** `internal/game/effects.go` (new), `internal/game/game.go` (add `Effects []Effect`), `internal/game/combat.go` (hook), `internal/game/effects_test.go` (new)
- `Effect`, `Game.Effects`, `AddEffect`, `tickEffects`, `EffectsSummary`. Call `g.tickEffects()` at the top of `monstersAct`; if `g.Dead` after it, return.
- Tests: poison damages each turn and can kill (cause "poison"); regen heals + clamps + expires after its turns; `AddEffect` refreshes to the longer duration.
- Commit: `feat(game): status-effect framework (poison, regen)`

## Task 2: behaviors — regenerate + eat_mushroom
**Files:** `internal/behaviors/behaviors.go`, `internal/behaviors/behaviors_test.go`
- `regenerate`: `AddEffect("regen", it.Def.Power)`. `eat_mushroom`: `restoreHP` then `AddEffect("poison", 6)`. Register both.
- Tests: regenerate adds a regen effect; eat_mushroom heals and adds poison.
- Commit: `feat(behaviors): regenerate + eat_mushroom (apply effects)`

## Task 3: ui — HUD shows active effects
**Files:** `internal/ui/ui.go` (statusLine), `internal/ui/ui_test.go`
- Append `[Poisoned, …]` to the status line when `EffectsSummary()` is non-empty.
- Test: a poisoned game shows "Poisoned" in the status.
- Commit: `feat(ui): show active status effects in the HUD`

## Task 4: data — mushroom + regen potion
**Files:** `data/items.toml`, `data/data_test.go`
- `red_mushroom` (food, use `eat_mushroom`, power 4, glyph `%`, red) and `potion_regeneration` (potion, use `regenerate`, power 8, glyph `!`, green).
- Golden test asserts both load.
- Commit: `feat(data): red mushroom (heal+poison) + regeneration potion`

## Task 5: verify, push, PR, loop
`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`. Commit plan. Push `status-effects`; PR documents the framework + that blindness/sleep/wands/scrolls follow. CodeRabbit loop → merge → pull master.

## Done when
Eating the red mushroom heals you but poisons you (HP drains for a few turns, shown in the HUD); a regeneration potion heals you over time. Suite green.
