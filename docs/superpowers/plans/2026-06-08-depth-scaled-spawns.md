# Depth-Scaled Spawns Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make monster spawning depth-aware (`min_depth`) so tougher monsters appear only on deeper floors — the first, highest-value slice of #16. Unblocks #13's deferred tanks.

**Architecture:** `MonsterDef.MinDepth` gates eligibility; `gen.Rooms` takes the current depth and `placeMonsters` filters the pool to `MinDepth <= depth`. The depth is already threaded by `Game.NewLevelFn(depth)` (cmd) and starts at 1. The big branching-named-level graph (the rest of #16) is a **later increment with its own design doc**; this slice keeps the linear depth model and `MaxDepth = 3`.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). Commands from `/Users/arda/projects/tsl/go`, prefixed `GOTOOLCHAIN=local`; prepend `env -u GOROOT` if a GOROOT error appears.

---

## Task 0: Branch
`depth-spawns` off `master`.

## Task 1: `min_depth` gating

**Files:** `internal/content/content.go`, `internal/gen/gen.go`, `cmd/tsl/main.go`, `internal/gen/gen_test.go`, `internal/content/content_test.go`

**Step 1 (test, gen):** Add to `gen_test.go`:
```go
func TestPlaceMonstersRespectsMinDepth(t *testing.T) {
	c := testContent()
	c.Monsters = map[string]*content.MonsterDef{
		"deepling": {ID: "deepling", Name: "deepling", Glyph: "D", Color: content.ColorRed, HP: 3, Attack: 1, Dodge: 1, Damage: "1d2", MinDepth: 5},
	}
	if l, _, _, err := Rooms(rng.NewWithSeed(7), c, 60, 24, 1); err != nil {
		t.Fatal(err)
	} else if len(l.Creatures) != 0 {
		t.Errorf("min_depth 5 monster must not spawn at depth 1, got %d", len(l.Creatures))
	}
	if l, _, _, err := Rooms(rng.NewWithSeed(7), c, 60, 24, 5); err != nil {
		t.Fatal(err)
	} else if len(l.Creatures) == 0 {
		t.Error("min_depth 5 monster should spawn at depth 5")
	}
}
```
Also update the existing `Rooms(...)` calls in `gen_test.go` to pass a trailing depth arg (`, 1` — except the new test which passes its own). The `TestRoomsPlacesMonsters` call should pass depth `1`.

**Step 2 (run, expect FAIL):**
```bash
GOTOOLCHAIN=local go test ./internal/gen/ -run TestPlaceMonstersRespectsMinDepth 2>&1 | tail
```
(Compile failure: `Rooms` arity + `MinDepth` field.)

**Step 3 (implement):**
- `content.go` `MonsterDef`: add `MinDepth int `toml:"min_depth"`` (after `Corpse`). In `validateMonster`, add `if m.MinDepth < 0 { return fmt.Errorf("min_depth must be >= 0, got %d", m.MinDepth) }`.
- `gen.go`: change signature to `func Rooms(r *rng.MT, c *content.Content, w, h, depth int) (...)` and pass `depth` to `placeMonsters(r, c, lvl, rooms, start, depth)`. In `placeMonsters(..., depth int)`, when building `ids`, `for id, m := range c.Monsters { if m.MinDepth > depth { continue }; ids = append(ids, id) }`.
- `cmd/tsl/main.go`: both `gen.Rooms(r, c, mapW, mapH)` calls → add depth. `newGame` initial level is depth 1: `gen.Rooms(r, c, mapW, mapH, 1)`. The `NewLevelFn` closure already receives `depth`: `gen.Rooms(r, c, mapW, mapH, depth)`.

**Step 4 (run, expect PASS):**
```bash
GOTOOLCHAIN=local go test ./internal/gen/ ./internal/content/ ./cmd/... 2>&1 | tail
```

**Step 5 (commit):** `feat(gen): depth-scaled monster spawning (min_depth)`

## Task 2: depth-gated tanks (data)

**Files:** `internal/gen/...` none; `data/monsters.toml`, `data/data_test.go`

**Step 1 (test):** extend `TestEmbeddedRoster` (or add `TestDepthGatedRoster`) asserting `c.Monsters["scarecrow"].MinDepth == 2` and `c.Monsters["slime"].MinDepth == 3`.

**Step 2 (run, expect FAIL).**

**Step 3 (data):** add to `monsters.toml` (stats inspired by C `monster.c`; glyphs from `glyph.c`; no corpse — straw/ooze isn't edible):
```toml
[monster.scarecrow]
name = "scarecrow"
glyph = "C"
color = "brown"
hp = 15
attack = 4
dodge = 0
damage = "1d6"
speed = 80
min_depth = 2

[monster.slime]
name = "slime"
glyph = "x"
color = "green"
hp = 18
attack = 3
dodge = 0
damage = "1d4"
speed = 80
min_depth = 3
```

**Step 4 (run, expect PASS)** — golden + full content validation.

**Step 5 (commit):** `feat(data): depth-gated tanks — scarecrow (d2), slime (d3)`

## Task 3: Verify, push, PR, loop
```bash
gofmt -l . && GOTOOLCHAIN=local go vet ./... && GOTOOLCHAIN=local go test ./... && GOTOOLCHAIN=local go build -o /tmp/tsl ./cmd/tsl && echo ALL_GREEN
```
Commit plan doc; push `depth-spawns`; `gh pr create` (part of #16, documents that this is the spawn-scaling slice; named branching levels to follow). Then CodeRabbit loop → merge `--delete-branch` → pull master.

## Done when
Deeper floors spawn the tanks (scarecrow d2+, slime d3); floor 1 stays early-tier; suite green. PR notes the named-level-graph remainder of #16 as the next increment.
