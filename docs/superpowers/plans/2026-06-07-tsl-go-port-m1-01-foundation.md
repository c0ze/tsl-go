# TSL Go Port — M1 Plan 1: Foundation & Walking Skeleton

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stand up the Go module and a data-driven "walking skeleton" — load tile definitions from TOML, build a static level, and walk an `@` around a room in a terminal — with tests at every layer.

**Architecture:** Approach A from the design spec (`docs/superpowers/specs/2026-06-07-tsl-go-port-design.md`). An I/O-free engine (`game`) owns state; content is loaded from TOML (`content`); the UI boundary (`ui`) is a `View` snapshot + a `Prompter`/`Renderer` pair the engine calls through; a tcell front-end implements them. The engine never imports the UI.

**Tech Stack:** Go 1.21, `github.com/gdamore/tcell/v2` (terminal), `github.com/BurntSushi/toml` (content). Standard `go test`.

---

## Working directory

The Go module lives in the `go/` subdirectory of this repo. **All `go` commands below are run from `go/`.** The original C source stays untouched as the behavior reference.

## This plan in context (M1 increments)

This is **Plan 1 of 6** for milestone M1 (the winnable console slice):

1. **Foundation & walking skeleton** ← this plan (module, data-driven tiles, static level, tcell render + movement)
2. RNG (Mersenne Twister + known-vector differential test) + dungeon generation → generated levels
3. Field of view + visibility/memory rendering
4. Creatures, monster content, AI (sleep/hunt), speed/energy scheduler, bump-to-attack, combat, HP/death, message log
5. Items, inventory/equip, behavior registry, a wand + heal effect, pickup/use UI prompts
6. Save/resume (JSON), win (reach exit) / lose (death + morgue), config & keymap

Each subsequent increment gets its own plan written when we reach it.

## File structure (this plan)

```text
go/
  go.mod                              module github.com/c0ze/tsl
  go.sum
  data/
    tiles.toml                        tile definitions (real content)
  cmd/tsl/
    main.go                           wiring: load content, build demo level, run loop
    main_test.go                      headless smoke test (scripted walk)
  internal/
    content/
      content.go                      TileDef, Color, Content, Load() + validation
      content_test.go                 load + validation-failure tests
    game/
      game.go                         Pos, Direction, Tile, Level, Game, Move()
      parse.go                        ParseLevel() — build a level from ASCII rows
      game_test.go                    movement, bounds, passability, ParseLevel
    ui/
      ui.go                           Cell, View, Action, Prompter, Renderer, BuildView, Run
      ui_test.go                      BuildView + Run (scripted) tests
      tcell/
        screen.go                     tcell Renderer+Prompter, key→action, color map
        screen_test.go                SimulationScreen render + key-mapping tests
```

Responsibilities are one-per-file; `game` has zero UI imports; `ui/tcell` is the only package that imports `tcell`.

---

## Task 1: Module scaffold

**Files:**
- Create: `go/go.mod`, `go/go.sum` (via tooling)
- Create: `data/tiles.toml`

- [ ] **Step 1: Create directories**

Run (from repo root):
```bash
mkdir -p cmd/tsl internal/content internal/game internal/ui/tcell data
```

- [ ] **Step 2: Initialize the module and add dependencies**

Run (from `go/`):
```bash
go mod init github.com/c0ze/tsl
go get github.com/BurntSushi/toml@v1.4.0
go get github.com/gdamore/tcell/v2@v2.7.4
```
> Pinned for Go 1.21 compatibility (tcell@latest requires Go 1.24). Run go commands with GOTOOLCHAIN=local.

Expected: `go.mod` and `go.sum` created; `go.mod` lists both requires.

- [ ] **Step 3: Create the real tile content file**

Create `data/tiles.toml`:
```toml
# Tile definitions. Each [tile.<id>] table becomes a TileDef keyed by <id>.
# glyph: exactly one character. color: one of normal/brown/blue/red/green/cyan/magenta/black.
[tile.floor]
glyph = "."
color = "normal"
passable = true
transparent = true

[tile.wall]
glyph = "#"
color = "normal"
passable = false
transparent = false
```

- [ ] **Step 4: Verify the module builds**

Run (from `go/`):
```bash
go build ./...
```
Expected: no output, exit 0 (no packages to build yet, but the module resolves).

- [ ] **Step 5: Commit**

```bash
git add go/go.mod go/go.sum data/tiles.toml
git commit -m "chore(go): scaffold Go module and tile content"
```

---

## Task 2: Content loader (`content` package)

**Files:**
- Create: `internal/content/content.go`
- Test: `internal/content/content_test.go`

- [ ] **Step 1: Write the failing tests**

Create `internal/content/content_test.go`:
```go
package content

import (
	"os"
	"path/filepath"
	"testing"
)

func writeTiles(t *testing.T, body string) string {
	t.Helper()
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte(body), 0o644); err != nil {
		t.Fatal(err)
	}
	return dir
}

func TestLoadTiles(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = "."
color = "normal"
passable = true
transparent = true

[tile.wall]
glyph = "#"
color = "normal"
passable = false
transparent = false
`)
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	floor, ok := c.Tiles["floor"]
	if !ok {
		t.Fatal("floor tile missing")
	}
	if floor.ID != "floor" {
		t.Errorf("ID = %q, want floor", floor.ID)
	}
	if floor.Rune() != '.' {
		t.Errorf("Rune = %q, want '.'", floor.Rune())
	}
	if !floor.Passable {
		t.Error("floor should be passable")
	}
	if c.Tiles["wall"].Passable {
		t.Error("wall should not be passable")
	}
}

func TestLoadRejectsBadColor(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = "."
color = "chartreuse"
passable = true
transparent = true
`)
	if _, err := Load(dir); err == nil {
		t.Fatal("expected error for invalid color, got nil")
	}
}

func TestLoadRejectsMultiRuneGlyph(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = ".."
color = "normal"
passable = true
transparent = true
`)
	if _, err := Load(dir); err == nil {
		t.Fatal("expected error for multi-rune glyph, got nil")
	}
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run (from `go/`):
```bash
go test ./internal/content/
```
Expected: FAIL — `undefined: Load` (build error).

- [ ] **Step 3: Write the implementation**

Create `internal/content/content.go`:
```go
// Package content loads validated game content (data) from TOML files.
package content

import (
	"fmt"
	"path/filepath"
	"unicode/utf8"

	"github.com/BurntSushi/toml"
)

// Color is a named glyph color mirroring the original game's palette.
type Color string

const (
	ColorNormal  Color = "normal"
	ColorBrown   Color = "brown"
	ColorBlue    Color = "blue"
	ColorRed     Color = "red"
	ColorGreen   Color = "green"
	ColorCyan    Color = "cyan"
	ColorMagenta Color = "magenta"
	ColorBlack   Color = "black"
)

var validColors = map[Color]bool{
	ColorNormal: true, ColorBrown: true, ColorBlue: true, ColorRed: true,
	ColorGreen: true, ColorCyan: true, ColorMagenta: true, ColorBlack: true,
}

// TileDef defines a kind of map tile.
type TileDef struct {
	ID          string `toml:"-"`
	Glyph       string `toml:"glyph"`
	Color       Color  `toml:"color"`
	Passable    bool   `toml:"passable"`
	Transparent bool   `toml:"transparent"`
}

// Rune returns the tile's glyph as a rune.
func (t *TileDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(t.Glyph)
	return r
}

// Content is the fully-loaded, validated game content.
type Content struct {
	Tiles map[string]*TileDef
}

type tilesFile struct {
	Tile map[string]*TileDef `toml:"tile"`
}

// Load reads and validates all content files in dir.
func Load(dir string) (*Content, error) {
	c := &Content{Tiles: map[string]*TileDef{}}

	var tf tilesFile
	path := filepath.Join(dir, "tiles.toml")
	if _, err := toml.DecodeFile(path, &tf); err != nil {
		return nil, fmt.Errorf("loading %s: %w", path, err)
	}
	for id, def := range tf.Tile {
		def.ID = id
		if err := validateTile(def); err != nil {
			return nil, fmt.Errorf("tile %q in %s: %w", id, path, err)
		}
		c.Tiles[id] = def
	}
	if len(c.Tiles) == 0 {
		return nil, fmt.Errorf("%s: no tiles defined", path)
	}
	return c, nil
}

func validateTile(t *TileDef) error {
	if utf8.RuneCountInString(t.Glyph) != 1 {
		return fmt.Errorf("glyph must be exactly one character, got %q", t.Glyph)
	}
	if !validColors[t.Color] {
		return fmt.Errorf("invalid color %q", t.Color)
	}
	return nil
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run (from `go/`):
```bash
go mod tidy
go test ./internal/content/
```
Expected: PASS (`ok github.com/c0ze/tsl/internal/content`).

- [ ] **Step 5: Commit**

```bash
git add internal/content/ go/go.mod go/go.sum
git commit -m "feat(content): TOML tile loader with validation"
```

---

## Task 3: Engine core model (`game` package)

**Files:**
- Create: `internal/game/game.go`
- Create: `internal/game/parse.go`
- Test: `internal/game/game_test.go`

- [ ] **Step 1: Write the failing tests**

Create `internal/game/game_test.go`:
```go
package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func testContent() *content.Content {
	return &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
	}}
}

func TestParseLevel(t *testing.T) {
	c := testContent()
	rows := []string{
		"###",
		"#@#",
		"###",
	}
	legend := map[rune]string{'#': "wall", '@': "floor"}
	lvl, start, err := ParseLevel(c, rows, legend)
	if err != nil {
		t.Fatalf("ParseLevel: %v", err)
	}
	if lvl.W != 3 || lvl.H != 3 {
		t.Fatalf("size = %dx%d, want 3x3", lvl.W, lvl.H)
	}
	if start != (Pos{1, 1}) {
		t.Errorf("start = %v, want {1 1}", start)
	}
	if lvl.Passable(Pos{0, 0}) {
		t.Error("corner wall should be impassable")
	}
	if !lvl.Passable(Pos{1, 1}) {
		t.Error("center floor should be passable")
	}
}

func TestMoveIntoFloorSucceeds(t *testing.T) {
	c := testContent()
	lvl, start, _ := ParseLevel(c, []string{"...", ".@.", "..."}, map[rune]string{'.': "floor", '@': "floor"})
	g := &Game{Content: c, Level: lvl, Player: start}
	if !g.Move(DirE) {
		t.Fatal("move east into floor should succeed")
	}
	if g.Player != (Pos{2, 1}) {
		t.Errorf("player = %v, want {2 1}", g.Player)
	}
}

func TestMoveIntoWallBlocked(t *testing.T) {
	c := testContent()
	lvl, start, _ := ParseLevel(c, []string{"###", "#@#", "###"}, map[rune]string{'#': "wall", '@': "floor"})
	g := &Game{Content: c, Level: lvl, Player: start}
	if g.Move(DirN) {
		t.Fatal("move into wall should be blocked")
	}
	if g.Player != (Pos{1, 1}) {
		t.Errorf("player moved to %v, want {1 1}", g.Player)
	}
}

func TestMoveOutOfBoundsBlocked(t *testing.T) {
	c := testContent()
	lvl, start, _ := ParseLevel(c, []string{"@."}, map[rune]string{'.': "floor", '@': "floor"})
	g := &Game{Content: c, Level: lvl, Player: start}
	if g.Move(DirW) {
		t.Fatal("move off the west edge should be blocked")
	}
}

func TestDirectionDelta(t *testing.T) {
	dx, dy := DirNE.Delta()
	if dx != 1 || dy != -1 {
		t.Errorf("DirNE delta = (%d,%d), want (1,-1)", dx, dy)
	}
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run (from `go/`):
```bash
go test ./internal/game/
```
Expected: FAIL — `undefined: ParseLevel`, `undefined: Game`, etc.

- [ ] **Step 3: Write the core model**

Create `internal/game/game.go`:
```go
// Package game holds the core world model and turn logic. It performs no I/O
// and never imports a UI.
package game

import "github.com/c0ze/tsl/internal/content"

// Pos is a grid coordinate.
type Pos struct{ X, Y int }

// Direction is one of the eight compass directions (or none).
type Direction int

const (
	DirNone Direction = iota
	DirN
	DirNE
	DirE
	DirSE
	DirS
	DirSW
	DirW
	DirNW
)

// Delta returns the (dx, dy) step for a direction.
func (d Direction) Delta() (int, int) {
	switch d {
	case DirN:
		return 0, -1
	case DirNE:
		return 1, -1
	case DirE:
		return 1, 0
	case DirSE:
		return 1, 1
	case DirS:
		return 0, 1
	case DirSW:
		return -1, 1
	case DirW:
		return -1, 0
	case DirNW:
		return -1, -1
	default:
		return 0, 0
	}
}

// Tile is a single map cell.
type Tile struct {
	Def *content.TileDef
}

// Level is a rectangular grid of tiles stored row-major.
type Level struct {
	W, H  int
	tiles []Tile
}

// InBounds reports whether p lies inside the level.
func (l *Level) InBounds(p Pos) bool {
	return p.X >= 0 && p.Y >= 0 && p.X < l.W && p.Y < l.H
}

// At returns the tile at p, which must be in bounds.
func (l *Level) At(p Pos) *Tile {
	return &l.tiles[p.Y*l.W+p.X]
}

// Passable reports whether p is in bounds and its tile can be walked into.
func (l *Level) Passable(p Pos) bool {
	return l.InBounds(p) && l.At(p).Def.Passable
}

// Game is the whole game state.
type Game struct {
	Content *content.Content
	Level   *Level
	Player  Pos
}

// Move attempts to move the player one step in d. It returns true if the player
// moved, false if the target was blocked or out of bounds.
func (g *Game) Move(d Direction) bool {
	dx, dy := d.Delta()
	dst := Pos{g.Player.X + dx, g.Player.Y + dy}
	if !g.Level.Passable(dst) {
		return false
	}
	g.Player = dst
	return true
}
```

- [ ] **Step 4: Write the level parser**

Create `internal/game/parse.go`:
```go
package game

import (
	"fmt"

	"github.com/c0ze/tsl/internal/content"
)

// ParseLevel builds a level from ASCII rows (all rows must be equal width and
// ASCII-only). Each rune is mapped through legend to a tile id that must exist
// in c. The '@' rune marks the player start; legend must also map '@' to the
// tile id that lies underneath the player. Returns the level and start position.
func ParseLevel(c *content.Content, rows []string, legend map[rune]string) (*Level, Pos, error) {
	h := len(rows)
	if h == 0 {
		return nil, Pos{}, fmt.Errorf("no rows")
	}
	w := len(rows[0])
	l := &Level{W: w, H: h, tiles: make([]Tile, w*h)}
	start := Pos{}
	for y, row := range rows {
		if len(row) != w {
			return nil, Pos{}, fmt.Errorf("row %d width %d, want %d", y, len(row), w)
		}
		for x := 0; x < len(row); x++ {
			r := rune(row[x])
			if r == '@' {
				start = Pos{x, y}
			}
			id, ok := legend[r]
			if !ok {
				return nil, Pos{}, fmt.Errorf("no legend entry for %q at (%d,%d)", r, x, y)
			}
			def, ok := c.Tiles[id]
			if !ok {
				return nil, Pos{}, fmt.Errorf("unknown tile id %q", id)
			}
			l.tiles[y*w+x] = Tile{Def: def}
		}
	}
	return l, start, nil
}
```

- [ ] **Step 5: Run the tests to verify they pass**

Run (from `go/`):
```bash
go test ./internal/game/
```
Expected: PASS (`ok github.com/c0ze/tsl/internal/game`).

- [ ] **Step 6: Commit**

```bash
git add internal/game/
git commit -m "feat(game): world model, movement, and ASCII level parser"
```

---

## Task 4: UI boundary (`ui` package)

**Files:**
- Create: `internal/ui/ui.go`
- Test: `internal/ui/ui_test.go`

- [ ] **Step 1: Write the failing tests**

Create `internal/ui/ui_test.go`:
```go
package ui

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func testGame(t *testing.T, rows []string) *game.Game {
	t.Helper()
	c := &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
	}}
	lvl, start, err := game.ParseLevel(c, rows, map[rune]string{'.': "floor", '#': "wall", '@': "floor"})
	if err != nil {
		t.Fatal(err)
	}
	return &game.Game{Content: c, Level: lvl, Player: start}
}

// scriptPrompter returns a fixed sequence of actions, then ActQuit forever.
type scriptPrompter struct {
	actions []Action
	i       int
}

func (s *scriptPrompter) NextAction() (Action, error) {
	if s.i >= len(s.actions) {
		return Action{Kind: ActQuit}, nil
	}
	a := s.actions[s.i]
	s.i++
	return a, nil
}

type nullRenderer struct{ frames int }

func (n *nullRenderer) Render(View) { n.frames++ }

func TestBuildViewPlacesPlayer(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
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

func TestRunAppliesActionsUntilQuit(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	p := &scriptPrompter{actions: []Action{
		{Kind: ActMove, Dir: game.DirE},
		{Kind: ActMove, Dir: game.DirE},
		{Kind: ActQuit},
	}}
	r := &nullRenderer{}
	if err := Run(g, p, r); err != nil {
		t.Fatalf("Run: %v", err)
	}
	if g.Player != (game.Pos{X: 3, Y: 1}) {
		t.Errorf("player = %v, want {3 1}", g.Player)
	}
	if r.frames == 0 {
		t.Error("expected at least one rendered frame")
	}
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run (from `go/`):
```bash
go test ./internal/ui/
```
Expected: FAIL — `undefined: BuildView`, `undefined: Run`, etc.

- [ ] **Step 3: Write the implementation**

Create `internal/ui/ui.go`:
```go
// Package ui defines the rendering/input boundary between the engine and any
// front-end. The engine never imports a concrete UI; front-ends implement
// Prompter and Renderer.
package ui

import (
	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

// Cell is one rendered grid cell.
type Cell struct {
	Glyph rune
	Color content.Color
}

// View is a read-only snapshot the front-end draws.
type View struct {
	W, H  int
	Cells []Cell // len W*H, row-major
}

// At returns a pointer to the cell at (x, y).
func (v *View) At(x, y int) *Cell { return &v.Cells[y*v.W+x] }

// ActionKind enumerates player intents.
type ActionKind int

const (
	ActNone ActionKind = iota
	ActMove
	ActQuit
)

// Action is a decoded player intent. Dir is meaningful only when Kind==ActMove.
type Action struct {
	Kind ActionKind
	Dir  game.Direction
}

// Prompter supplies player actions to the game loop.
type Prompter interface {
	NextAction() (Action, error)
}

// Renderer draws a View.
type Renderer interface {
	Render(View)
}

// Player avatar rendering (Plan 1; later the player becomes a creature def).
const PlayerGlyph = '@'

// PlayerColor is the player's glyph color.
var PlayerColor = content.ColorNormal

// BuildView produces the View for the current game state.
func BuildView(g *game.Game) View {
	l := g.Level
	v := View{W: l.W, H: l.H, Cells: make([]Cell, l.W*l.H)}
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			def := l.At(game.Pos{X: x, Y: y}).Def
			*v.At(x, y) = Cell{Glyph: def.Rune(), Color: def.Color}
		}
	}
	*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: PlayerGlyph, Color: PlayerColor}
	return v
}

// Run is the core game loop: render, get an action, apply it, until quit.
func Run(g *game.Game, p Prompter, r Renderer) error {
	for {
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

- [ ] **Step 4: Run the tests to verify they pass**

Run (from `go/`):
```bash
go test ./internal/ui/
```
Expected: PASS (`ok github.com/c0ze/tsl/internal/ui`).

- [ ] **Step 5: Commit**

```bash
git add internal/ui/ui.go internal/ui/ui_test.go
git commit -m "feat(ui): View/Prompter/Renderer boundary and game loop"
```

---

## Task 5: tcell front-end (`ui/tcell` package)

**Files:**
- Create: `internal/ui/tcell/screen.go`
- Test: `internal/ui/tcell/screen_test.go`

- [ ] **Step 1: Write the failing tests**

Create `internal/ui/tcell/screen_test.go`:
```go
package tcell

import (
	"testing"

	tc "github.com/gdamore/tcell/v2"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/ui"
)

func TestRenderToSimulationScreen(t *testing.T) {
	sim := tc.NewSimulationScreen("")
	if err := sim.Init(); err != nil {
		t.Fatal(err)
	}
	defer sim.Fini()
	sim.SetSize(3, 3)

	sc := NewWith(sim)
	v := ui.View{W: 3, H: 3, Cells: make([]ui.Cell, 9)}
	for i := range v.Cells {
		v.Cells[i] = ui.Cell{Glyph: '.', Color: content.ColorNormal}
	}
	*v.At(1, 1) = ui.Cell{Glyph: '@', Color: content.ColorNormal}
	sc.Render(v)

	cells, w, _ := sim.GetContents()
	if got := cells[1*w+1].Runes[0]; got != '@' {
		t.Errorf("cell (1,1) = %q, want '@'", got)
	}
	if got := cells[0].Runes[0]; got != '.' {
		t.Errorf("cell (0,0) = %q, want '.'", got)
	}
}

func TestKeyToAction(t *testing.T) {
	cases := []struct {
		ev   *tc.EventKey
		want ui.Action
		ok   bool
	}{
		{tc.NewEventKey(tc.KeyRune, 'l', tc.ModNone), ui.Action{Kind: ui.ActMove, Dir: game.DirE}, true},
		{tc.NewEventKey(tc.KeyRune, 'k', tc.ModNone), ui.Action{Kind: ui.ActMove, Dir: game.DirN}, true},
		{tc.NewEventKey(tc.KeyLeft, 0, tc.ModNone), ui.Action{Kind: ui.ActMove, Dir: game.DirW}, true},
		{tc.NewEventKey(tc.KeyRune, 'q', tc.ModNone), ui.Action{Kind: ui.ActQuit}, true},
		{tc.NewEventKey(tc.KeyRune, 'z', tc.ModNone), ui.Action{}, false},
	}
	for _, tt := range cases {
		got, ok := keyToAction(tt.ev)
		if ok != tt.ok || got != tt.want {
			t.Errorf("keyToAction(%v) = (%v,%v), want (%v,%v)", tt.ev.Name(), got, ok, tt.want, tt.ok)
		}
	}
}
```

- [ ] **Step 2: Run the tests to verify they fail**

Run (from `go/`):
```bash
go test ./internal/ui/tcell/
```
Expected: FAIL — `undefined: NewWith`, `undefined: keyToAction`.

- [ ] **Step 3: Write the implementation**

Create `internal/ui/tcell/screen.go`:
```go
// Package tcell renders the game to a terminal using gdamore/tcell and maps
// key events to UI actions. It is the only package that imports tcell.
package tcell

import (
	tc "github.com/gdamore/tcell/v2"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/ui"
)

// Screen is a tcell-backed ui.Renderer and ui.Prompter.
type Screen struct {
	s tc.Screen
}

// New creates and initializes a real terminal screen.
func New() (*Screen, error) {
	s, err := tc.NewScreen()
	if err != nil {
		return nil, err
	}
	if err := s.Init(); err != nil {
		return nil, err
	}
	return &Screen{s: s}, nil
}

// NewWith wraps an existing tcell.Screen (used in tests with a SimulationScreen).
func NewWith(s tc.Screen) *Screen { return &Screen{s: s} }

// Close restores the terminal.
func (sc *Screen) Close() { sc.s.Fini() }

var colorMap = map[content.Color]tc.Color{
	content.ColorNormal:  tc.ColorWhite,
	content.ColorBrown:   tc.ColorMaroon,
	content.ColorBlue:    tc.ColorBlue,
	content.ColorRed:     tc.ColorRed,
	content.ColorGreen:   tc.ColorGreen,
	content.ColorCyan:    tc.ColorTeal,
	content.ColorMagenta: tc.ColorFuchsia,
	content.ColorBlack:   tc.ColorBlack,
}

// Render draws the view and flushes it to the screen.
func (sc *Screen) Render(v ui.View) {
	sc.s.Clear()
	for y := 0; y < v.H; y++ {
		for x := 0; x < v.W; x++ {
			c := v.At(x, y)
			st := tc.StyleDefault.Foreground(colorMap[c.Color])
			sc.s.SetContent(x, y, c.Glyph, nil, st)
		}
	}
	sc.s.Show()
}

// NextAction blocks for a key event and maps it to a ui.Action.
func (sc *Screen) NextAction() (ui.Action, error) {
	for {
		switch ev := sc.s.PollEvent().(type) {
		case *tc.EventKey:
			if a, ok := keyToAction(ev); ok {
				return a, nil
			}
		case *tc.EventResize:
			sc.s.Sync()
		}
	}
}

func keyToAction(ev *tc.EventKey) (ui.Action, bool) {
	switch ev.Key() {
	case tc.KeyUp:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirN}, true
	case tc.KeyDown:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirS}, true
	case tc.KeyLeft:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirW}, true
	case tc.KeyRight:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirE}, true
	}
	switch ev.Rune() {
	case 'h':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirW}, true
	case 'j':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirS}, true
	case 'k':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirN}, true
	case 'l':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirE}, true
	case 'y':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirNW}, true
	case 'u':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirNE}, true
	case 'b':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirSW}, true
	case 'n':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirSE}, true
	case 'q':
		return ui.Action{Kind: ui.ActQuit}, true
	}
	return ui.Action{}, false
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run (from `go/`):
```bash
go test ./internal/ui/tcell/
```
Expected: PASS (`ok github.com/c0ze/tsl/internal/ui/tcell`).

- [ ] **Step 5: Commit**

```bash
git add internal/ui/tcell/
git commit -m "feat(ui/tcell): terminal renderer and key mapping"
```

---

## Task 6: Wiring + headless smoke test (`cmd/tsl`)

**Files:**
- Create: `cmd/tsl/main.go`
- Test: `cmd/tsl/main_test.go`

- [ ] **Step 1: Write the failing test**

Create `cmd/tsl/main_test.go`:
```go
package main

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/ui"
)

type scriptPrompter struct {
	actions []ui.Action
	i       int
}

func (s *scriptPrompter) NextAction() (ui.Action, error) {
	if s.i >= len(s.actions) {
		return ui.Action{Kind: ui.ActQuit}, nil
	}
	a := s.actions[s.i]
	s.i++
	return a, nil
}

type nullRenderer struct{}

func (nullRenderer) Render(ui.View) {}

func TestDemoGameWalk(t *testing.T) {
	c := &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
	}}
	g, err := demoGame(c)
	if err != nil {
		t.Fatalf("demoGame: %v", err)
	}
	// '@' starts at (6,3). Move east twice to (8,3); a third east hits the wall at (9,3).
	p := &scriptPrompter{actions: []ui.Action{
		{Kind: ui.ActMove, Dir: game.DirE},
		{Kind: ui.ActMove, Dir: game.DirE},
		{Kind: ui.ActMove, Dir: game.DirE},
		{Kind: ui.ActQuit},
	}}
	if err := ui.Run(g, p, nullRenderer{}); err != nil {
		t.Fatalf("Run: %v", err)
	}
	if g.Player != (game.Pos{X: 8, Y: 3}) {
		t.Errorf("player = %v, want {8 3}", g.Player)
	}
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run (from `go/`):
```bash
go test ./cmd/tsl/
```
Expected: FAIL — `undefined: demoGame`.

- [ ] **Step 3: Write main.go**

Create `cmd/tsl/main.go`:
```go
// Command tsl is the console front-end for the Go port of The Slimy Lichmummy.
package main

import (
	"fmt"
	"os"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/ui"
	tcellui "github.com/c0ze/tsl/internal/ui/tcell"
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, "tsl:", err)
		os.Exit(1)
	}
}

func run() error {
	c, err := content.Load("data")
	if err != nil {
		return err
	}
	g, err := demoGame(c)
	if err != nil {
		return err
	}
	screen, err := tcellui.New()
	if err != nil {
		return err
	}
	defer screen.Close()
	return ui.Run(g, screen, screen)
}

// demoGame builds a small hand-made room to walk around in. This is a Plan 1
// placeholder; dungeon generation replaces it in Plan 2.
func demoGame(c *content.Content) (*game.Game, error) {
	rows := []string{
		"##########",
		"#........#",
		"#..####..#",
		"#..#..@..#",
		"#..####..#",
		"#........#",
		"##########",
	}
	legend := map[rune]string{'#': "wall", '.': "floor", '@': "floor"}
	lvl, start, err := game.ParseLevel(c, rows, legend)
	if err != nil {
		return nil, err
	}
	return &game.Game{Content: c, Level: lvl, Player: start}, nil
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run (from `go/`):
```bash
go test ./cmd/tsl/
```
Expected: PASS (`ok github.com/c0ze/tsl/cmd/tsl`).

- [ ] **Step 5: Verify the whole module builds and all tests pass**

Run (from `go/`):
```bash
go build ./...
go test ./...
go vet ./...
```
Expected: build succeeds; all packages `ok`; vet clean.

- [ ] **Step 6: Manual smoke check (optional but recommended)**

Run (from `go/`):
```bash
go run ./cmd/tsl
```
Expected: a terminal shows the room with `@`; `h/j/k/l` and arrows move it, walls block it, `q` quits. (Run in a real terminal, not a captured pipe.)

- [ ] **Step 7: Commit**

```bash
git add cmd/tsl/
git commit -m "feat(cmd): wire walking skeleton with headless smoke test"
```

---

## Done criteria for Plan 1

- `go test ./...` passes from `go/`.
- `go run ./cmd/tsl` shows a walkable room (`@` moves with vi-keys + arrows, walls block, `q` quits).
- Tile content is loaded and validated from `data/tiles.toml`.
- The `game` package imports no UI; `ui/tcell` is the only tcell importer.

This establishes every architectural seam (content → engine → view → front-end) that the remaining M1 increments build on.
