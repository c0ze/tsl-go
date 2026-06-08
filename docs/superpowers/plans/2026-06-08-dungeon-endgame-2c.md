# Dungeon Endgame 2c Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Turn the Chapel into the real ending — an **ascension altar** you step onto to win, guarded by the **elder mummylich**, with the **Dragon** in the Dragons Lair. Completes #16 and delivers the iconic 0.40 finale.

**Architecture:** A tile can be a win tile (`TileDef.win`); stepping onto it wins. A level def gains `altar` (place a win tile) and `boss` (place one guaranteed monster). This **replaces** the temporary `LevelDef.win` (arrival-wins) with reach-the-altar-wins. Fail-fast validation keeps content honest.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). Commands run from the `go/` directory at the repo root. TDD, commit per task, suite green between commits.

## Task 1: content — win tile + level altar/boss

**Files:** `internal/content/content.go`, `internal/content/content_test.go`

- `TileDef`: add `Win bool` (toml `win`) — stepping onto it wins.
- `LevelDef`: **remove** `Win`; add `Altar bool` (toml `altar`) and `Boss string` (toml `boss`).
- `validateLevels`: if `l.Boss != ""`, require it names a defined monster; if `l.Altar`, require a tile id `altar` exists with `Win == true`.

Tests: a level with `boss` to an unknown monster is rejected; `altar = true` with no win-tile defined is rejected; a valid altar+boss level loads.

Commit: `feat(content): win tile + level altar/boss (replaces arrival-win)`

## Task 2: engine — win on stepping onto a win tile

**Files:** `internal/game/combat.go` (PlayerStep), `internal/game/dungeon.go` (Travel), `internal/game/dungeon_test.go`, `internal/ui/ui_test.go`

- In `PlayerStep`, after a successful `Move`, if the new tile's `Def.Win` is set: set `g.Won`, log `"You ascend to demigodhood. You win!"`, and return (no monster turn).
- In `Travel`, **remove** the `def.Win` arrival-win block.
- Update `dungeon_test.go`: drop `TestTravelWinsOnWinLevel` + the `bWins` param; keep move + persistence tests. Add (in the game package) a step-onto-win-tile test.
- Update `ui_test.go`: `travelGame` no longer sets a win level; rename the win test to assert travel changes the current level (the altar win is covered in the game package).

Commit: `feat(game): win by stepping onto an altar (win) tile`

## Task 3: gen — place boss + altar

**Files:** `internal/gen/gen.go`, `internal/gen/gen_test.go`

- In `LevelFromDef`: if `def.Boss != ""`, place one `c.Monsters[def.Boss]` in a distinct non-start room (full HP). If `def.Altar`, set a distinct room's center to `c.Tiles["altar"]`.
- Tests: a def with `boss` places exactly one boss of that id; a def with `altar` puts a win tile on the map.

Commit: `feat(gen): place guaranteed boss + ascension altar`

## Task 4: data — altar, bosses, Chapel & Lair

**Files:** `data/tiles.toml`, `data/monsters.toml`, `data/levels.toml`, `data/data_test.go`

- `tiles.toml`: `altar` tile (glyph `_`, cyan, passable, transparent, `win = true`).
- `monsters.toml`: `elder_mummylich` (glyph `E`, magenta, hp 40, attack 6, dodge 3, damage `1d8`, speed 110) and `dragon` (glyph `D`, red, hp 50, attack 7, dodge 3, damage `1d10`, speed 110).
- `levels.toml`: Chapel — remove `win = true`, add `altar = true` and `boss = "elder_mummylich"`. Dragons Lair — add `boss = "dragon"`.
- `data_test.go`: `TestEmbeddedDungeon` — Chapel has `Altar` + boss `elder_mummylich`, no `Win`; Dragons Lair boss `dragon`; the `altar` tile is a win tile.

Commit: `feat(data): ascension altar + elder mummylich + dragon`

## Task 5: verify, push, PR, loop

`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`. Commit plan. Push `dungeon-endgame`; PR documents the finale (altar win + bosses), notes balance is tunable (bosses are deadly but the win is reach-the-altar, so beatable without killing them) and that gear breadth (#14) will make the fights fairer. CodeRabbit loop → merge → pull master.

## Done when
Reaching the Chapel, you face the elder mummylich and can step onto the `_` altar to win with "You ascend to demigodhood." The Dragon guards the Dragons Lair. Suite green; closes #16.
