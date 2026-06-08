# Status-Effect Wands 15d Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** A wand that applies a timed status effect to its target instead of (or
as well as) blasting it for damage. Concretely: a *wand of venom* that poisons
the creature you aim at, so it dies over the next several turns. This generalizes
the status-effect system from player-only to any creature, unlocking all future
creature afflictions (slow, sleep, fear) and effect-bearing items.

## Design
- **Effects on creatures.** Today `Effect{Kind,Turns}` lives only on the player
  (`Game.Effects`). Extract the add/refresh logic into a free function
  `addEffect(effects, kind, turns) []Effect`; `Game.AddEffect` and a new
  `Creature.AddEffect` both delegate to it. Give `Creature` an `Effects []Effect`.
- **Ticking creature effects.** `tickCreatureEffects(m) bool` applies each
  effect's per-turn outcome (poison: `HP--`), decrements/expires, and when the
  creature reaches 0 HP resolves death through the existing `killCreature`
  (drops a corpse, removes it) and reports `true`. `monstersAct` ticks each
  living creature once per turn, before it gains energy; a creature that died of
  poison is skipped.
- **Effect-bearing items.** Mirror `TileDef`: add `ItemDef.Effect string` +
  `ItemDef.EffectTurns int` (`effect` / `effect_turns`). A trap tile and a venom
  wand now describe an affliction the same way.
- **ZapWand.** Spend a charge; if a creature is on the target, deal the wand's
  `damage` (only when it has a damage spec) and, if it survives, apply the wand's
  `effect` for `effect_turns`. A wand may carry damage, an effect, or both.

**Scope:** poison is the one creature effect wired this pass (symmetric with the
player's poison). The mechanism is general — later effects (slow/sleep) only add
a `case` to the tick and, for control effects, a check in the monster's action.
No per-creature status shown in the HUD yet (the zap message announces it).

## Task 1: content — effect-bearing items
**Files:** `internal/content/content.go`, `internal/content/content_test.go`
- Add `Effect string` (`effect`) + `EffectTurns int` (`effect_turns`) to `ItemDef`.
- `validateItem`: any item with `effect != ""` needs `effect_turns > 0`
  (mirrors `validateTile`). A `wand` now needs a valid `damage` spec **or** an
  effect; with a damage spec present it must still parse.
- Tests (first, watch fail): effect-only wand loads; wand with neither damage
  nor effect rejected; `effect` without `effect_turns` rejected; existing
  damage-only wand still loads.
- Commit: `feat(content): effect-bearing items (effect/effect_turns)`

## Task 2: game — creature effects + zap effect path
**Files:** `internal/game/creature.go` (Effects field + AddEffect),
`internal/game/effects.go` (addEffect free fn, tickCreatureEffects),
`internal/game/combat.go` (monstersAct tick, ZapWand effect), tests
- Extract `addEffect`; `Game.AddEffect`/`Creature.AddEffect` delegate (player
  effect tests stay green).
- `tickCreatureEffects(m *Creature) bool`: poison `HP--`, expire, `killCreature`
  + return `true` on death.
- `monstersAct`: for each snapshot creature still present, tick its effects;
  `continue` if it died.
- `ZapWand`: only roll/apply damage when `it.Def.Damage != ""`; if the target
  survives and the wand has an `Effect`, `m.AddEffect(effect, turns)` and log it.
- Tests (first): creature poison damages + kills over ticks; `Creature.AddEffect`
  refreshes to longer; zapping a venom wand poisons a live target and spends a
  charge; a poisoned monster dies across `monstersAct` turns.
- Commit: `feat(game): status effects on creatures + effect wands`

## Task 3: data — wand of venom
**Files:** `data/items.toml`, `data/data_test.go`
- `wand_venom` (kind wand, glyph `/`, green, `effect = "poison"`,
  `effect_turns = 8`, `power = 5` = charges, no damage). Golden test
  `TestEmbeddedVenomWand`.
- Commit: `feat(data): wand of venom (poisons its target)`

## Task 4: verify, push, PR, loop
`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`.
Push `effect-wands`; open PR. CodeRabbit loop → merge when green → pull master.

## Done when
Aim a wand of venom at a monster: it takes no immediate hit but withers and dies
over the next handful of turns. Damage wands are unaffected. Suite green.
