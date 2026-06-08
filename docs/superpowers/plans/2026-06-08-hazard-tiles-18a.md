# Hazard Tiles 18a Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Traps — a tile that applies a status effect when stepped on. First #18 slice, built on the #15 effect system. Faithful to 0.40's traps (visible for now; hidden/detection later).

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). Commands run from the `go/` directory at the repo root. TDD, commit per task, suite green between commits.

## Design
- `TileDef` gains `Effect string` (a status-effect kind) + `EffectTurns int`. Stepping onto such a tile calls `AddEffect(Effect, EffectTurns)`.
- `LevelDef` gains `Traps int` — how many `dart_trap` tiles the generator scatters.
- Validation: a tile with `Effect` set needs `EffectTurns > 0`; a level with `Traps > 0` needs a `dart_trap` tile defined (with an effect), so bad content fails at load (not mid-game).

## Task 1: content — tile effect + level traps
**Files:** `internal/content/content.go`, `internal/content/content_test.go`
- `TileDef.Effect` (`toml:"effect"`), `TileDef.EffectTurns` (`toml:"effect_turns"`). `validateTile`: if `Effect != ""`, require `EffectTurns > 0`.
- `LevelDef.Traps` (`toml:"traps"`). In `validateLevels`: `Traps >= 0`; if `Traps > 0`, require `c.Tiles["dart_trap"]` to exist with a non-empty `Effect`.
- Tests: effect tile loads; effect with 0 turns rejected; level traps>0 without a dart_trap tile rejected.
- Commit: `feat(content): tile status-effect + level trap count`

## Task 2: game — apply tile effect on step
**Files:** `internal/game/combat.go` (PlayerStep), `internal/game/combat_test.go`
- After a successful `Move`, if the tile's `Def.Effect != ""`, `AddEffect(Def.Effect, Def.EffectTurns)` and log "You trigger a trap!".
- Test: stepping onto an effect tile applies the effect.
- Commit: `feat(game): stepping onto a hazard tile applies its effect`

## Task 3: gen — scatter traps
**Files:** `internal/gen/gen.go`, `internal/gen/gen_test.go`
- In `LevelFromDef`, place `def.Traps` `dart_trap` tiles on passable, non-start, non-portal floor tiles.
- Test: a def with `Traps: N` and a dart_trap tile produces tiles whose `Def.Effect != ""`.
- Commit: `feat(gen): scatter trap tiles per level`

## Task 4: data — dart_trap + level traps
**Files:** `data/tiles.toml`, `data/levels.toml`, `data/data_test.go`
- `dart_trap` tile (glyph `^`, red, passable, transparent, `effect = "poison"`, `effect_turns = 5`).
- Add `traps` counts to a few deeper levels (e.g. catacombs 3, laboratory 4, dragons_lair 5).
- Golden: the dart_trap tile is a poison effect tile.
- Commit: `feat(data): dart trap (poison) + traps on deeper levels`

## Task 5: verify, push, PR, loop
`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`. Commit plan. Push `hazard-tiles`; PR notes hidden/detection traps + doors/light are later #18 slices. CodeRabbit loop → merge → pull master.

## Done when
Deeper floors hide `^` dart traps that poison you on contact (shown in the HUD). Suite green.
