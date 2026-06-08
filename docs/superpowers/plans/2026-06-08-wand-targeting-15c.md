# Wands + Targeting 15c Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Interactive targeting + wands — the biggest missing mechanic. Press `z`, pick a wand, aim a cursor at a creature, and blast it. A force-bolt wand that damages a targeted creature.

## Design
- **Targeting** is a new `Prompter.Target(origin game.Pos) (game.Pos, bool)`: the front-end shows a movable cursor over the last-rendered map; arrows/hjkl move it, Enter confirms, Esc cancels.
- **Wands** are `kind = "wand"` items with a `damage` dice spec and per-instance **charges** (`Item.Charges`, seeded from the def's `power` when generated). `Game.ZapWand(it, target)` spends a charge, rolls damage against the creature at the target (kills via `killCreature`), and passes a turn.
- **Run** gets an `ActZap` (`z`): filter inventory to wands → `Menu` to pick → `Target` → `ZapWand`.

**Scope:** instant hit on the target tile's creature (no beam path / line-of-sight blocking yet); damage wands only. Later: status-effect wands (monster effects), beams, charges UI, auto-target nearest.

## Task 1: content — wand kind
**Files:** `internal/content/content.go`, `internal/content/content_test.go`
- Add `"wand"` to `validItemKinds`; in `validateItem`, a `wand` needs a valid `damage` spec (like a weapon). Tests: wand loads; bad-damage wand rejected.
- Commit: `feat(content): wand item kind`

## Task 2: game — charges + ZapWand
**Files:** `internal/game/item.go` (Item.Charges), `internal/game/use.go` (WandInventory), `internal/game/combat.go` (ZapWand), tests
- `Item.Charges int`. `WandInventory() []*Item`. `ZapWand(it *Item, target Pos)`: if `Charges <= 0` log "no charges" (no turn); else spend a charge, `dmg := RNG.RollSpec(it.Def.Damage)`, hit the creature at target (`killCreature` if dead) or "fizzles", then `monstersAct()`.
- Tests: zap damages + kills a creature; spends a charge; empty wand does nothing.
- Commit: `feat(game): wand charges + ZapWand (targeted damage)`

## Task 3: gen — seed wand charges
**Files:** `internal/gen/gen.go` (placeItems), `internal/gen/gen_test.go`
- When placing a `wand` item, set `Charges = Def.Power`. Test: a placed wand has charges.
- Commit: `feat(gen): seed wand charges from power`

## Task 4: ui — targeting + ActZap
**Files:** `internal/ui/ui.go`, `internal/ui/ui_test.go`, `internal/ui/tcell/screen.go`, `internal/ui/tcell/screen_test.go`
- `Prompter.Target(origin game.Pos) (game.Pos, bool)`; `ActZap` in the enum; `Run` ActZap case (wand menu → target → ZapWand). Add `Target` to the test prompters.
- tcell: `Screen.last ui.View` saved in `Render`; `Target` redraws `last` + a `*` cursor, moves on arrows/hjkl, Enter/Esc; `z` → `ActZap` in `keyToAction`.
- Tests: ui Run zaps a creature via a fake Target; tcell `z` → ActZap.
- Commit: `feat(ui): interactive targeting + z to zap a wand`

## Task 5: data — force-bolt wand
**Files:** `data/items.toml`, `data/data_test.go`
- `wand_force_bolt` (kind wand, glyph `/`, blue, damage `2d6`, power 5 = charges). Golden test.
- Commit: `feat(data): wand of force bolt`

## Task 6: verify, push, PR, loop
`gofmt -l . && go vet ./... && go test ./... && go build -o /tmp/tsl ./cmd/tsl`. Push `wand-targeting`; PR. CodeRabbit loop → merge → pull master.

## Done when
Find a wand, press `z`, aim at a monster, and blast it (charges deplete). Suite green.
