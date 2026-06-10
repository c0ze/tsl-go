# Player turn-energy scheduler (16a) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Port the C tick scheduler to the player. In 0.40 (`game.c play()` /
`increment_counters()`) *every* creature — the player included — banks
`attr_speed` into `move_counter` each tick and acts when it reaches `TURN_TIME`;
the player is just another creature with `BASE_SPEED` 100. Our engine already
does this for monsters (`Energy`/`speedOf`) but the player implicitly acts once
per world advance, so nothing can ever speed up or slow down the *player*. This
PR makes the player a first-class energy citizen, unlocking haste/slow on the
player and, next, potions of speed/slowing (16b) and sleep. Multi-PR arc; this
is the engine half.

## C grounding
- `game.c:590-600`: each tick `move_counter += attr_current(attr_speed)`; a
  creature whose counter reaches `TURN_TIME` (1000) acts and pays it back.
- `rules.h`: `BASE_SPEED 100`, `HASTE_AMOUNT 30`, `HASTE_LENGTH 20`,
  `SLOW_LENGTH 20`.
- `effect.c std_effect()`: `effect_haste` = speed +30; `effect_slow` = speed −1
  (vestigial — a −1 on a 100 scale is noise; our shipped monster slow halves
  speed, so the player mirrors that for one consistent rule).
- Per-**turn** vs per-**tick**: `pass_time_on_effects` (effect TTLs) and
  `energy()` (EP regen) run only when a creature actually takes its turn
  (`game.c:150-175`), so the player's effect clocks and EP regen stay per
  player-turn, not per world tick.
- `play()` opens with `goto creature_turn` for the player: the player always
  moves first. We keep that via the surplus convention below — a zero surplus
  means "exactly ready to act", so a fresh game's zero value needs no builder
  initialization.

## Design
- `Game.playerEnergy int`: the player's surplus beyond the turn just taken (the
  C's `move_counter − TURN_TIME`), so the zero value means ready to act first.
- `playerSpeed()`: base 100; `haste` → +30 (`hasteBonus`, C `HASTE_AMOUNT`);
  `slow` → halved (same rule monsters use in `monstersAct`); floor 1.
- `advanceWorld()` — the per-player-turn entry, replacing `monstersAct()` at all
  8 production call sites (PlayerStep/ZapWand/fire/spell×3/use×2):
  `tickEffects()` + `regenEP()` once, pay `turnCost`, then
  `for playerEnergy < 0 { playerEnergy += playerSpeed(); worldTick() }`.
- `worldTick()` — old `monstersAct` minus the player bookkeeping: monster effect
  ticks + energy banking + actions. Tests that drove single ticks via
  `monstersAct()` rename mechanically.
- At speed 100 exactly one tick passes per turn — existing behavior is
  unchanged. Slowed: two ticks per turn (monsters act twice). Hasted: every few
  turns zero ticks pass (a free player action), averaging 1.3 actions per tick.
- `haste` joins `effectLabels` ("Hastened").

## Task 1: scheduler (TDD)
**Files:** `internal/game/scheduler_test.go` (new), `internal/game/combat.go`,
`internal/game/game.go`, `internal/game/effects.go`, callers in
`fire.go`/`spell.go`/`use.go`, builder in `build.go`.
- Tests first (red): baseline — one `advanceWorld()` moves a distant rat 1 tile;
  slowed player — 2 tiles; hasted player — 4 turns close 4 tiles, the 5th is a
  free action (rat frozen), the 6th moves again; slowed player's own poison
  still burns 1 HP per player turn (TTLs are per-turn, not per-tick).
- Implement (green): `playerEnergy`/`playerSpeed()`/`advanceWorld()`/`worldTick()`,
  swap call sites, rename in tests.
- Commit: `feat(game): player turn-energy scheduler (tick loop)`

## Task 2: gate + ship
- gofmt + go vet + full `go test ./...`; branch, push, PR, CodeRabbit loop →
  merge with merge commit.

## Done when
A slowed player watches every monster move twice between his steps; a hasted
player periodically gets a free action. An unaffected game plays exactly as
before. Suite green.
