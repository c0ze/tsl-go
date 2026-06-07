# TSL Go Port — M1 Plan 3: Field of View + Visibility/Memory

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** The player only sees a lit area around them; explored-but-unseen tiles are remembered (dimmed); unexplored tiles are dark. FOV recomputes each turn.

**Architecture:** New leaf package `internal/fov` exposing `Compute(grid, ox, oy, radius, visit)` via a `Grid` interface (no game/UI imports). `internal/game` tracks per-tile `Visible`/`Seen`, adapts `Level` to `fov.Grid`, and recomputes via `Game.UpdateFOV()`. `internal/ui` renders visible tiles bright, seen tiles dim, unseen as blank, and the loop calls `UpdateFOV` each turn. `internal/ui/tcell` applies a dim style.

**FOV algorithm note:** This uses **recursive shadowcasting** (the standard roguelike algorithm), a deliberate, reversible divergence from `common/fov.c`'s idiosyncratic bouncing-ray + dilation approach. Same player experience (radius-limited circular sight, walls block LOS); clean and testable. The `fov.Compute` seam lets a bit-faithful port of `fov.c` replace it later without touching callers, should exact 0.40 FOV be wanted.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). No new dependencies.

## Working directory & environment
- Module in `go/`; run go commands from there with `export GOTOOLCHAIN=local`.
- Branch: `plan3-fov`. Do NOT run `go mod tidy` / `go get`.

## File structure (this plan)
```text
go/
  internal/
    fov/
      fov.go        Grid interface + Compute (recursive shadowcasting)
      fov_test.go   origin/open-field/wall-blocking tests
    game/
      game.go       Tile gains Visible, Seen
      fov.go        VisionRadius, fovGrid adapter, (*Game).UpdateFOV()  (new file)
      fov_test.go   UpdateFOV open-field + wall-blocking tests
    ui/
      ui.go         Cell.Dim; BuildView visibility logic; Run calls UpdateFOV
      ui_test.go    updated BuildView test + visibility-states test
      tcell/
        screen.go   Render applies a dim style for Cell.Dim
```

---

## Task 1: FOV package (`internal/fov`)

**Files:** Create `go/internal/fov/fov.go`, `go/internal/fov/fov_test.go`

- [ ] **Step 1: Failing tests** — `go/internal/fov/fov_test.go`:
```go
package fov

import "testing"

type testGrid struct {
	w, h   int
	opaque map[[2]int]bool
}

func (g testGrid) InBounds(x, y int) bool { return x >= 0 && y >= 0 && x < g.w && y < g.h }
func (g testGrid) Opaque(x, y int) bool   { return g.opaque[[2]int{x, y}] }

func collect(g Grid, ox, oy, r int) map[[2]int]bool {
	seen := map[[2]int]bool{}
	Compute(g, ox, oy, r, func(x, y int) { seen[[2]int{x, y}] = true })
	return seen
}

func TestComputeOriginAlwaysVisible(t *testing.T) {
	seen := collect(testGrid{w: 5, h: 5}, 2, 2, 3)
	if !seen[[2]int{2, 2}] {
		t.Error("origin must be visible")
	}
}

func TestComputeOpenFieldDisk(t *testing.T) {
	seen := collect(testGrid{w: 21, h: 21}, 10, 10, 4)
	if !seen[[2]int{14, 10}] {
		t.Error("tile at radius 4 should be visible")
	}
	if seen[[2]int{16, 10}] {
		t.Error("tile beyond radius should not be visible")
	}
}

func TestComputeWallColumnBlocks(t *testing.T) {
	g := testGrid{w: 11, h: 3, opaque: map[[2]int]bool{
		{5, 0}: true, {5, 1}: true, {5, 2}: true,
	}}
	seen := collect(g, 1, 1, 8)
	if !seen[[2]int{5, 1}] {
		t.Error("the wall itself should be visible")
	}
	if seen[[2]int{9, 1}] {
		t.Error("tile behind the wall column should be blocked")
	}
}
```

- [ ] **Step 2: Run → FAIL** (`undefined: Compute`): `export GOTOOLCHAIN=local && go test ./internal/fov/`

- [ ] **Step 3: Implement** — `go/internal/fov/fov.go`:
```go
// Package fov computes field of view via recursive shadowcasting. It depends on
// nothing but the caller-supplied Grid, so it is a pure leaf package.
package fov

// Grid is the read-only map view FOV needs.
type Grid interface {
	InBounds(x, y int) bool
	Opaque(x, y int) bool // does (x,y) block sight?
}

// multipliers for transforming coordinates into the 8 octants.
var mult = [4][8]int{
	{1, 0, 0, -1, -1, 0, 0, 1},
	{0, 1, -1, 0, 0, -1, 1, 0},
	{0, 1, 1, 0, 0, -1, -1, 0},
	{1, 0, 0, 1, -1, 0, 0, -1},
}

// Compute calls visit(x,y) once for every tile visible from (ox,oy) within
// radius (Euclidean), including the origin and any opaque tiles that are seen.
func Compute(g Grid, ox, oy, radius int, visit func(x, y int)) {
	if visit == nil || radius < 0 {
		return
	}
	visit(ox, oy)
	for oct := 0; oct < 8; oct++ {
		castLight(g, ox, oy, radius, 1, 1.0, 0.0,
			mult[0][oct], mult[1][oct], mult[2][oct], mult[3][oct], visit)
	}
}

func castLight(g Grid, cx, cy, radius, row int, start, end float64, xx, xy, yx, yy int, visit func(x, y int)) {
	if start < end {
		return
	}
	radiusSq := radius * radius
	newStart := 0.0
	blocked := false
	for j := row; j <= radius && !blocked; j++ {
		dy := -j
		for dx := -j; dx <= 0; dx++ {
			lSlope := (float64(dx) - 0.5) / (float64(dy) + 0.5)
			rSlope := (float64(dx) + 0.5) / (float64(dy) - 0.5)
			if start < rSlope {
				continue
			}
			if end > lSlope {
				break
			}
			mx := cx + dx*xx + dy*xy
			my := cy + dx*yx + dy*yy
			inBounds := g.InBounds(mx, my)
			if dx*dx+dy*dy <= radiusSq && inBounds {
				visit(mx, my)
			}
			// Out-of-bounds counts as opaque so light can't leak past map edges.
			opaque := !inBounds || g.Opaque(mx, my)
			if blocked {
				if opaque {
					newStart = rSlope
					continue
				}
				blocked = false
				start = newStart
			} else if opaque && j < radius {
				blocked = true
				castLight(g, cx, cy, radius, j+1, start, lSlope, xx, xy, yx, yy, visit)
				newStart = rSlope
			}
		}
	}
}
```

- [ ] **Step 4: Run → PASS**: `export GOTOOLCHAIN=local && go test ./internal/fov/`

- [ ] **Step 5: Commit**
```bash
git add go/internal/fov/
git commit -m "feat(fov): recursive shadowcasting field of view"
```

---

## Task 2: Per-tile visibility + UpdateFOV (`internal/game`)

**Files:** Modify `go/internal/game/game.go`; create `go/internal/game/fov.go`, `go/internal/game/fov_test.go`

- [ ] **Step 1: Extend the Tile struct** in `go/internal/game/game.go` — replace:
```go
// Tile is a single map cell.
type Tile struct {
	Def *content.TileDef
}
```
with:
```go
// Tile is a single map cell. Visible is true when currently in the player's
// FOV; Seen is true once it has ever been visible (remembered/dimmed).
type Tile struct {
	Def     *content.TileDef
	Visible bool
	Seen    bool
}
```

- [ ] **Step 2: Failing tests** — `go/internal/game/fov_test.go`:
```go
package game

import "testing"

func TestUpdateFOVOpenField(t *testing.T) {
	c := testContent()
	l := NewLevel(21, 21, c.Tiles["floor"])
	g := &Game{Content: c, Level: l, Player: Pos{X: 10, Y: 10}}
	g.UpdateFOV()
	if !l.At(Pos{X: 10, Y: 10}).Visible || !l.At(Pos{X: 10, Y: 10}).Seen {
		t.Error("player tile should be visible and seen")
	}
	if !l.At(Pos{X: 12, Y: 10}).Visible {
		t.Error("nearby tile should be visible")
	}
	if l.At(Pos{X: 0, Y: 0}).Visible {
		t.Error("far corner (beyond radius) should not be visible")
	}
}

func TestUpdateFOVWallBlocks(t *testing.T) {
	c := testContent()
	l := NewLevel(11, 3, c.Tiles["floor"])
	for y := 0; y < 3; y++ {
		l.Set(Pos{X: 5, Y: y}, c.Tiles["wall"])
	}
	g := &Game{Content: c, Level: l, Player: Pos{X: 1, Y: 1}}
	g.UpdateFOV()
	if !l.At(Pos{X: 5, Y: 1}).Visible {
		t.Error("the wall itself should be visible")
	}
	if l.At(Pos{X: 9, Y: 1}).Visible {
		t.Error("tile behind the wall should be blocked")
	}
}

func TestUpdateFOVClearsStaleVisible(t *testing.T) {
	c := testContent()
	l := NewLevel(21, 21, c.Tiles["floor"])
	g := &Game{Content: c, Level: l, Player: Pos{X: 1, Y: 1}}
	g.UpdateFOV()
	far := Pos{X: 19, Y: 19}
	if l.At(far).Visible {
		t.Fatal("precondition: far tile should not be visible from corner")
	}
	// Move near the far tile and recompute: previously-visible tiles near the
	// old position must no longer be Visible (but remain Seen).
	near := Pos{X: 2, Y: 1}
	if !l.At(near).Visible {
		t.Fatal("precondition: near tile visible from start")
	}
	g.Player = far
	g.UpdateFOV()
	if l.At(near).Visible {
		t.Error("stale tile should no longer be Visible after moving away")
	}
	if !l.At(near).Seen {
		t.Error("tile should remain Seen after leaving FOV")
	}
}
```

- [ ] **Step 3: Run → FAIL** (`undefined: UpdateFOV`): `export GOTOOLCHAIN=local && go test ./internal/game/`

- [ ] **Step 4: Implement** — `go/internal/game/fov.go`:
```go
package game

import "github.com/c0ze/tsl/internal/fov"

// VisionRadius is how far the player can see, in tiles. It becomes
// attribute-driven once creatures land in a later plan.
const VisionRadius = 8

// fovGrid adapts a Level to fov.Grid.
type fovGrid struct{ l *Level }

func (f fovGrid) InBounds(x, y int) bool { return f.l.InBounds(Pos{X: x, Y: y}) }

func (f fovGrid) Opaque(x, y int) bool {
	p := Pos{X: x, Y: y}
	return !f.l.InBounds(p) || !f.l.At(p).Def.Transparent
}

// UpdateFOV recomputes which tiles are currently visible from the player and
// marks them Seen. Tiles outside the new FOV are cleared to not-Visible but
// keep their Seen state (so they render dimmed from memory).
func (g *Game) UpdateFOV() {
	for i := range g.Level.tiles {
		g.Level.tiles[i].Visible = false
	}
	fov.Compute(fovGrid{g.Level}, g.Player.X, g.Player.Y, VisionRadius, func(x, y int) {
		t := g.Level.At(Pos{X: x, Y: y})
		t.Visible = true
		t.Seen = true
	})
}
```

- [ ] **Step 5: Run → PASS**: `export GOTOOLCHAIN=local && go test ./internal/game/`

- [ ] **Step 6: Commit**
```bash
git add go/internal/game/game.go go/internal/game/fov.go go/internal/game/fov_test.go
git commit -m "feat(game): per-tile visibility/memory and UpdateFOV"
```

---

## Task 3: Visibility-aware rendering (`internal/ui`)

**Files:** Modify `go/internal/ui/ui.go`, `go/internal/ui/ui_test.go`

- [ ] **Step 1: Update tests** — in `go/internal/ui/ui_test.go`, (a) make `TestBuildViewPlacesPlayer` compute FOV first, and (b) add a visibility-states test. Replace the existing `TestBuildViewPlacesPlayer` with:
```go
func TestBuildViewPlacesPlayer(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
	g.UpdateFOV()
	v := BuildView(g)
	if v.W != 3 || v.H != 3 {
		t.Fatalf("view size = %dx%d, want 3x3", v.W, v.H)
	}
	if got := v.At(1, 1).Glyph; got != '@' {
		t.Errorf("center glyph = %q, want '@'", got)
	}
	if got := v.At(0, 0).Glyph; got != '.' {
		t.Errorf("corner glyph = %q, want '.'", got)
	}
}

func TestBuildViewVisibilityStates(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			tl := g.Level.At(game.Pos{X: x, Y: y})
			tl.Visible, tl.Seen = false, false
		}
	}
	g.Level.At(game.Pos{X: 1, Y: 1}).Visible = true // player tile
	vis := g.Level.At(game.Pos{X: 0, Y: 1})
	vis.Visible, vis.Seen = true, true
	g.Level.At(game.Pos{X: 0, Y: 0}).Seen = true // remembered only
	// (2,2) stays fully unseen

	v := BuildView(g)
	if c := v.At(1, 1); c.Glyph != '@' {
		t.Errorf("player cell = %q, want '@'", c.Glyph)
	}
	if c := v.At(0, 1); c.Glyph != '.' || c.Dim {
		t.Errorf("visible floor = %q dim=%v, want '.' bright", c.Glyph, c.Dim)
	}
	if c := v.At(0, 0); c.Glyph != '.' || !c.Dim {
		t.Errorf("remembered floor = %q dim=%v, want '.' dim", c.Glyph, c.Dim)
	}
	if c := v.At(2, 2); c.Glyph != ' ' {
		t.Errorf("unseen cell = %q, want blank space", c.Glyph)
	}
}
```

- [ ] **Step 2: Run → FAIL** (Cell has no `Dim`): `export GOTOOLCHAIN=local && go test ./internal/ui/`

- [ ] **Step 3: Implement** — in `go/internal/ui/ui.go`:

(a) add `Dim` to `Cell`:
```go
// Cell is one rendered grid cell.
type Cell struct {
	Glyph rune
	Color content.Color
	Dim   bool // render dimmed (remembered-but-not-currently-visible)
}
```

(b) replace `BuildView` with the visibility-aware version:
```go
// BuildView produces the View for the current game state: tiles in the player's
// FOV are drawn bright, remembered (Seen) tiles dim, and unseen tiles blank.
func BuildView(g *game.Game) View {
	l := g.Level
	v := View{W: l.W, H: l.H, Cells: make([]Cell, l.W*l.H)}
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			t := l.At(game.Pos{X: x, Y: y})
			switch {
			case t.Visible:
				*v.At(x, y) = Cell{Glyph: t.Def.Rune(), Color: t.Def.Color}
			case t.Seen:
				*v.At(x, y) = Cell{Glyph: t.Def.Rune(), Color: t.Def.Color, Dim: true}
			default:
				*v.At(x, y) = Cell{Glyph: ' ', Color: content.ColorNormal}
			}
		}
	}
	*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: PlayerGlyph, Color: PlayerColor}
	return v
}
```

(c) make `Run` recompute FOV each turn:
```go
// Run is the core game loop: recompute FOV, render, get an action, apply it.
func Run(g *game.Game, p Prompter, r Renderer) error {
	for {
		g.UpdateFOV()
		r.Render(BuildView(g))
		a, err := p.NextAction()
		if err != nil {
			return err
		}
		switch a.Kind {
		case ActQuit:
			return nil
		case ActMove:
			g.Move(a.Dir)
		}
	}
}
```

- [ ] **Step 4: Run → PASS**: `export GOTOOLCHAIN=local && go test ./internal/ui/`

- [ ] **Step 5: Commit**
```bash
git add go/internal/ui/ui.go go/internal/ui/ui_test.go
git commit -m "feat(ui): render FOV — bright/dim/blank by visibility"
```

---

## Task 4: Dim rendering in the tcell front-end (`internal/ui/tcell`)

**Files:** Modify `go/internal/ui/tcell/screen.go`

- [ ] **Step 1: Implement** — in `go/internal/ui/tcell/screen.go`, update `Render` to apply a dim style:
```go
// Render draws the view and flushes it to the screen.
func (sc *Screen) Render(v ui.View) {
	sc.s.Clear()
	for y := 0; y < v.H; y++ {
		for x := 0; x < v.W; x++ {
			c := v.At(x, y)
			st := tc.StyleDefault.Foreground(colorMap[c.Color])
			if c.Dim {
				st = st.Dim(true)
			}
			sc.s.SetContent(x, y, c.Glyph, nil, st)
		}
	}
	sc.s.Show()
}
```

- [ ] **Step 2: Verify whole module** — from `go/`:
```bash
export GOTOOLCHAIN=local
gofmt -l .
go build ./... && go test ./... -count=1 && go vet ./...
go build -o /tmp/tsl_p3 ./cmd/tsl && echo "binary OK" && rm -f /tmp/tsl_p3
```
Expected: gofmt lists nothing, all packages `ok`, vet clean, binary builds. (`go run ./cmd/tsl` now reveals the dungeon as you explore.)

- [ ] **Step 3: Commit**
```bash
git add go/internal/ui/tcell/screen.go
git commit -m "feat(ui/tcell): dim style for remembered tiles"
```

---

## Done criteria (Plan 3)
- `internal/fov.Compute` (shadowcasting): origin visible, open-field disk bounded by radius, wall columns block sight (tested).
- `Game.UpdateFOV` marks current FOV Visible+Seen and clears stale Visible while keeping Seen (tested).
- `BuildView` renders bright/dim/blank by visibility; `Run` recomputes FOV each turn.
- `go run ./cmd/tsl` reveals the generated dungeon progressively as the player explores; remembered areas stay dimmed.
- `go test ./...`, `go vet ./...`, gofmt all clean.

## Deferred
- Per-creature/attribute-driven vision radius (arrives with creatures in Plan 4).
- Light sources, see-through-able special tiles, telepathy/detection — later.
- A bit-faithful port of `common/fov.c`'s bouncing-ray FOV remains an option behind `fov.Compute` if exact 0.40 parity is later desired.
