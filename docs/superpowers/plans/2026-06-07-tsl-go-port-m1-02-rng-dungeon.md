# TSL Go Port — M1 Plan 2: RNG + Dungeon Generation

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Replace Plan 1's hand-made static room with a procedurally generated dungeon: port the Mersenne Twister RNG (bit-exact vs. the C) and add a seeded rooms-and-corridors generator, then wire generated levels into `cmd/tsl`.

**Architecture:** Per the approved design spec (`docs/superpowers/specs/2026-06-07-tsl-go-port-design.md`, §6/§7). New leaf package `internal/rng` (faithful MT19937 port for reproducibility). New package `internal/gen` builds `game.Level`s from a seeded `*rng.MT`. A small exported level-building API is added to `internal/game`. Dependency flow stays one-way: `cmd → gen → {game, rng, content}`; `game → {content, ...}`; `rng` is a pure leaf.

**Tech Stack:** Go 1.21 (toolchain pinned; run go commands with `GOTOOLCHAIN=local`). Deps already pinned (tcell v2.7.4, toml v1.4.0). No new dependencies.

---

## Working directory & environment

- Go module is in `go/`; run all `go` commands from there with `export GOTOOLCHAIN=local`.
- Branch: `plan2-rng-gen` (already created). Commit directly to it.
- Do NOT run `go mod tidy` or `go get` (no new deps; pins must stay).
- The C reference for the RNG is `common/mt19937ar.c` (the canonical MT19937; its commented `main()` documents the test vector used below).

## File structure (this plan)

```text
go/
  data/
    tiles.toml                 + [tile.stairs_down]
  internal/
    rng/
      mt.go                    MT19937 port: MT, NewWithSeed, NewWithKey, Uint32, Intn
      mt_test.go               known-vector differential test + determinism + Intn range
    gen/
      gen.go                   Rooms() rooms-and-corridors generator
      gen_test.go              deterministic + connectivity + start/stairs invariants
    game/
      build.go                 + NewLevel(), (*Level).Set()  (new file)
      build_test.go            NewLevel/Set tests
  cmd/tsl/
    main.go                    demoGame → newGame(seed) using gen
    main_test.go               TestDemoGameWalk → TestNewGamePlayable
```

---

## Task 1: Mersenne Twister RNG (`internal/rng`)

**Files:**
- Create: `go/internal/rng/mt.go`
- Test: `go/internal/rng/mt_test.go`

- [ ] **Step 1: Write the failing tests** — create `go/internal/rng/mt_test.go`:
```go
package rng

import "testing"

// Canonical MT19937 output for init_by_array({0x123,0x234,0x345,0x456})
// (see the commented main() in common/mt19937ar.c / the reference mt19937ar.out).
func TestKnownVector(t *testing.T) {
	g := NewWithKey([]uint32{0x123, 0x234, 0x345, 0x456})
	// Verified against the reference C (common/mt19937ar.c) compiled and run.
	want := []uint32{
		1067595299, 955945823, 477289528, 4107218783, 4228976476,
		3344332714, 3355579695, 227628506,
	}
	for i, w := range want {
		if got := g.Uint32(); got != w {
			t.Fatalf("Uint32 #%d = %d, want %d", i, got, w)
		}
	}
}

func TestSeedDeterministic(t *testing.T) {
	a := NewWithSeed(12345)
	b := NewWithSeed(12345)
	for i := 0; i < 100; i++ {
		if x, y := a.Uint32(), b.Uint32(); x != y {
			t.Fatalf("same seed diverged at %d: %d != %d", i, x, y)
		}
	}
}

func TestIntnRange(t *testing.T) {
	g := NewWithSeed(1)
	for i := 0; i < 10000; i++ {
		v := g.Intn(7)
		if v < 0 || v >= 7 {
			t.Fatalf("Intn(7) = %d, out of [0,7)", v)
		}
	}
}

func TestIntnPanicsOnNonPositive(t *testing.T) {
	defer func() {
		if recover() == nil {
			t.Fatal("expected panic for Intn(0)")
		}
	}()
	NewWithSeed(1).Intn(0)
}

func TestNewWithKeyPanicsOnEmptyKey(t *testing.T) {
	defer func() {
		if recover() == nil {
			t.Fatal("expected panic for NewWithKey([])")
		}
	}()
	NewWithKey([]uint32{})
}
```

- [ ] **Step 2: Run to verify they fail** — from `go/`: `export GOTOOLCHAIN=local && go test ./internal/rng/` → FAIL (`undefined: NewWithKey`).

- [ ] **Step 3: Write the implementation** — create `go/internal/rng/mt.go`:
```go
// Package rng is a faithful Go port of the Mersenne Twister MT19937 PRNG
// (mt19937ar.c by Matsumoto & Nishimura), used so dungeon generation is
// reproducible from a seed. Ported from common/mt19937ar.c; Uint32 reproduces
// genrand_int32 bit-for-bit. Not safe for concurrent use.
package rng

const (
	n         = 624
	m         = 397
	matrixA   = 0x9908b0df
	upperMask = 0x80000000
	lowerMask = 0x7fffffff
)

// MT is a Mersenne Twister generator.
type MT struct {
	mt  [n]uint32
	mti int
}

// NewWithSeed seeds with a single 32-bit value (init_genrand).
func NewWithSeed(s uint32) *MT {
	g := &MT{}
	g.initGenrand(s)
	return g
}

// NewWithKey seeds with a key array (init_by_array). It panics on an empty key.
func NewWithKey(key []uint32) *MT {
	if len(key) == 0 {
		panic("rng: NewWithKey requires a non-empty key")
	}
	g := &MT{}
	g.initByArray(key)
	return g
}

func (g *MT) initGenrand(s uint32) {
	g.mt[0] = s
	for i := 1; i < n; i++ {
		g.mt[i] = 1812433253*(g.mt[i-1]^(g.mt[i-1]>>30)) + uint32(i)
	}
	g.mti = n
}

func (g *MT) initByArray(key []uint32) {
	g.initGenrand(19650218)
	i, j := 1, 0
	k := n
	if len(key) > k {
		k = len(key)
	}
	for ; k > 0; k-- {
		g.mt[i] = (g.mt[i] ^ ((g.mt[i-1] ^ (g.mt[i-1] >> 30)) * 1664525)) + key[j] + uint32(j)
		i++
		j++
		if i >= n {
			g.mt[0] = g.mt[n-1]
			i = 1
		}
		if j >= len(key) {
			j = 0
		}
	}
	for k = n - 1; k > 0; k-- {
		g.mt[i] = (g.mt[i] ^ ((g.mt[i-1] ^ (g.mt[i-1] >> 30)) * 1566083941)) - uint32(i)
		i++
		if i >= n {
			g.mt[0] = g.mt[n-1]
			i = 1
		}
	}
	g.mt[0] = 0x80000000
}

// Uint32 returns the next value on [0, 2^32) (genrand_int32).
func (g *MT) Uint32() uint32 {
	var y uint32
	mag01 := [2]uint32{0, matrixA}
	if g.mti >= n {
		if g.mti == n+1 {
			g.initGenrand(5489)
		}
		var kk int
		for kk = 0; kk < n-m; kk++ {
			y = (g.mt[kk] & upperMask) | (g.mt[kk+1] & lowerMask)
			g.mt[kk] = g.mt[kk+m] ^ (y >> 1) ^ mag01[y&1]
		}
		for ; kk < n-1; kk++ {
			y = (g.mt[kk] & upperMask) | (g.mt[kk+1] & lowerMask)
			g.mt[kk] = g.mt[kk+(m-n)] ^ (y >> 1) ^ mag01[y&1]
		}
		y = (g.mt[n-1] & upperMask) | (g.mt[0] & lowerMask)
		g.mt[n-1] = g.mt[m-1] ^ (y >> 1) ^ mag01[y&1]
		g.mti = 0
	}
	y = g.mt[g.mti]
	g.mti++
	y ^= y >> 11
	y ^= (y << 7) & 0x9d2c5680
	y ^= (y << 15) & 0xefc60000
	y ^= y >> 18
	return y
}

// Intn returns a uniformly distributed int in [0, nn). It panics if nn <= 0.
// Rejection sampling removes modulo bias.
func (g *MT) Intn(nn int) int {
	if nn <= 0 {
		panic("rng: Intn requires n > 0")
	}
	if uint64(nn) > uint64(^uint32(0)) {
		panic("rng: Intn requires n <= 2^32-1")
	}
	u := uint32(nn)
	thresh := (-u) % u // == 2^32 mod u: reject the low biased range
	for {
		v := g.Uint32()
		if v >= thresh {
			return int(v % u)
		}
	}
}
```

- [ ] **Step 4: Run to verify they pass** — from `go/`: `export GOTOOLCHAIN=local && go test ./internal/rng/` → PASS.

- [ ] **Step 5: Commit**
```bash
git add go/internal/rng/
git commit -m "feat(rng): faithful MT19937 port with known-vector test"
```

---

## Task 2: Level-building API + stairs tile (`internal/game`, `data`)

**Files:**
- Create: `go/internal/game/build.go`
- Test: `go/internal/game/build_test.go`
- Modify: `go/data/tiles.toml`

- [ ] **Step 1: Add the stairs-down tile** — append to `go/data/tiles.toml`:
```toml

[tile.stairs_down]
glyph = ">"
color = "normal"
passable = true
transparent = true
```

- [ ] **Step 2: Write the failing tests** — create `go/internal/game/build_test.go`:
```go
package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestNewLevelFills(t *testing.T) {
	wall := &content.TileDef{ID: "wall", Glyph: "#", Color: content.ColorNormal}
	l := NewLevel(4, 3, wall)
	if l.W != 4 || l.H != 3 {
		t.Fatalf("size = %dx%d, want 4x3", l.W, l.H)
	}
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			if l.At(Pos{x, y}).Def != wall {
				t.Fatalf("tile (%d,%d) not filled with wall", x, y)
			}
		}
	}
}

func TestSetReplacesTile(t *testing.T) {
	wall := &content.TileDef{ID: "wall", Glyph: "#", Passable: false}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Passable: true}
	l := NewLevel(3, 3, wall)
	l.Set(Pos{1, 1}, floor)
	if l.At(Pos{1, 1}).Def != floor {
		t.Error("Set did not replace the tile def")
	}
	if !l.Passable(Pos{1, 1}) {
		t.Error("center should be passable after Set(floor)")
	}
	if l.Passable(Pos{0, 0}) {
		t.Error("corner should still be wall")
	}
}
```

- [ ] **Step 3: Run to verify they fail** — from `go/`: `export GOTOOLCHAIN=local && go test ./internal/game/` → FAIL (`undefined: NewLevel`).

- [ ] **Step 4: Write the implementation** — create `go/internal/game/build.go`:
```go
package game

import "github.com/c0ze/tsl/internal/content"

// NewLevel returns a level of size w×h with every tile set to fill.
func NewLevel(w, h int, fill *content.TileDef) *Level {
	l := &Level{W: w, H: h, tiles: make([]Tile, w*h)}
	for i := range l.tiles {
		l.tiles[i] = Tile{Def: fill}
	}
	return l
}

// Set replaces the tile definition at p, which must be in bounds.
func (l *Level) Set(p Pos, def *content.TileDef) {
	l.At(p).Def = def
}
```

- [ ] **Step 5: Run to verify they pass** — from `go/`: `export GOTOOLCHAIN=local && go test ./internal/game/` → PASS (existing game tests plus the two new ones).

- [ ] **Step 6: Commit**
```bash
git add go/internal/game/build.go go/internal/game/build_test.go go/data/tiles.toml
git commit -m "feat(game): NewLevel/Set level-building API + stairs_down tile"
```

---

## Task 3: Rooms-and-corridors generator (`internal/gen`)

**Files:**
- Create: `go/internal/gen/gen.go`
- Test: `go/internal/gen/gen_test.go`

- [ ] **Step 1: Write the failing tests** — create `go/internal/gen/gen_test.go`:
```go
package gen

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/rng"
)

func testContent() *content.Content {
	return &content.Content{Tiles: map[string]*content.TileDef{
		"floor":       {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":        {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
		"stairs_down": {ID: "stairs_down", Glyph: ">", Color: content.ColorNormal, Passable: true, Transparent: true},
	}}
}

// bfsReaches reports whether dst is reachable from src over passable tiles.
func bfsReaches(l *game.Level, src, dst game.Pos) bool {
	seen := map[game.Pos]bool{src: true}
	queue := []game.Pos{src}
	dirs := []game.Direction{game.DirN, game.DirE, game.DirS, game.DirW}
	for len(queue) > 0 {
		p := queue[0]
		queue = queue[1:]
		if p == dst {
			return true
		}
		for _, d := range dirs {
			dx, dy := d.Delta()
			np := game.Pos{X: p.X + dx, Y: p.Y + dy}
			if !seen[np] && l.Passable(np) {
				seen[np] = true
				queue = append(queue, np)
			}
		}
	}
	return false
}

func glyphGrid(l *game.Level) string {
	b := make([]rune, 0, (l.W+1)*l.H)
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			b = append(b, l.At(game.Pos{X: x, Y: y}).Def.Rune())
		}
		b = append(b, '\n')
	}
	return string(b)
}

func TestRoomsDeterministic(t *testing.T) {
	c := testContent()
	l1, s1, d1, err := Rooms(rng.NewWithSeed(42), c, 60, 24)
	if err != nil {
		t.Fatal(err)
	}
	l2, s2, d2, err := Rooms(rng.NewWithSeed(42), c, 60, 24)
	if err != nil {
		t.Fatal(err)
	}
	if s1 != s2 || d1 != d2 {
		t.Fatalf("non-deterministic start/stairs: %v/%v vs %v/%v", s1, d1, s2, d2)
	}
	if glyphGrid(l1) != glyphGrid(l2) {
		t.Fatal("same seed produced different level layouts")
	}
}

func TestRoomsConnectivityAndPlacement(t *testing.T) {
	c := testContent()
	for seed := uint32(1); seed <= 20; seed++ {
		l, start, down, err := Rooms(rng.NewWithSeed(seed), c, 60, 24)
		if err != nil {
			t.Fatalf("seed %d: %v", seed, err)
		}
		if !l.InBounds(start) || !l.InBounds(down) {
			t.Fatalf("seed %d: start/down out of bounds", seed)
		}
		if !l.Passable(start) {
			t.Fatalf("seed %d: start %v not passable", seed, start)
		}
		if l.At(down).Def.ID != "stairs_down" {
			t.Fatalf("seed %d: down tile is %q, want stairs_down", seed, l.At(down).Def.ID)
		}
		if !bfsReaches(l, start, down) {
			t.Fatalf("seed %d: stairs not reachable from start", seed)
		}
	}
}
```

- [ ] **Step 2: Run to verify they fail** — from `go/`: `export GOTOOLCHAIN=local && go test ./internal/gen/` → FAIL (`undefined: Rooms`).

- [ ] **Step 3: Write the implementation** — create `go/internal/gen/gen.go`:
```go
// Package gen builds dungeon levels procedurally from a seeded *rng.MT, so a
// given seed always yields the same level.
package gen

import (
	"fmt"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/rng"
)

const (
	maxRooms = 12
	minRoomW = 4
	maxRoomW = 10
	minRoomH = 3
	maxRoomH = 7
)

type rect struct{ x, y, w, h int }

func (r rect) center() game.Pos { return game.Pos{X: r.x + r.w/2, Y: r.y + r.h/2} }

// intersects reports whether r and o overlap, treating a 1-tile gap as touching
// so rooms keep a wall between them.
func (r rect) intersects(o rect) bool {
	return r.x <= o.x+o.w && r.x+r.w >= o.x && r.y <= o.y+o.h && r.y+r.h >= o.y
}

// Rooms generates a rooms-and-corridors level of size w×h. It returns the level,
// the player start (center of the first room), and the down-stairs position
// (center of the last room). Each room is joined to the previous by a corridor,
// so the level is fully connected.
func Rooms(r *rng.MT, c *content.Content, w, h int) (*game.Level, game.Pos, game.Pos, error) {
	floor, wall, stairs := c.Tiles["floor"], c.Tiles["wall"], c.Tiles["stairs_down"]
	if floor == nil || wall == nil || stairs == nil {
		return nil, game.Pos{}, game.Pos{}, fmt.Errorf("gen: tiles floor/wall/stairs_down must all be defined")
	}

	lvl := game.NewLevel(w, h, wall)
	var rooms []rect
	for i := 0; i < maxRooms; i++ {
		rw := minRoomW + r.Intn(maxRoomW-minRoomW+1)
		rh := minRoomH + r.Intn(maxRoomH-minRoomH+1)
		if w-rw-1 < 1 || h-rh-1 < 1 {
			continue
		}
		room := rect{1 + r.Intn(w-rw-1), 1 + r.Intn(h-rh-1), rw, rh}
		overlaps := false
		for _, o := range rooms {
			if room.intersects(o) {
				overlaps = true
				break
			}
		}
		if overlaps {
			continue
		}
		carveRoom(lvl, room, floor)
		if len(rooms) > 0 {
			carveCorridor(r, lvl, rooms[len(rooms)-1].center(), room.center(), floor)
		}
		rooms = append(rooms, room)
	}
	if len(rooms) == 0 {
		return nil, game.Pos{}, game.Pos{}, fmt.Errorf("gen: no rooms placed (level too small)")
	}

	start := rooms[0].center()
	down := rooms[len(rooms)-1].center()
	lvl.Set(down, stairs)
	return lvl, start, down, nil
}

func carveRoom(lvl *game.Level, r rect, floor *content.TileDef) {
	for y := r.y; y < r.y+r.h; y++ {
		for x := r.x; x < r.x+r.w; x++ {
			lvl.Set(game.Pos{X: x, Y: y}, floor)
		}
	}
}

func carveCorridor(r *rng.MT, lvl *game.Level, a, b game.Pos, floor *content.TileDef) {
	if r.Intn(2) == 0 {
		carveH(lvl, a.X, b.X, a.Y, floor)
		carveV(lvl, a.Y, b.Y, b.X, floor)
	} else {
		carveV(lvl, a.Y, b.Y, a.X, floor)
		carveH(lvl, a.X, b.X, b.Y, floor)
	}
}

func carveH(lvl *game.Level, x0, x1, y int, floor *content.TileDef) {
	if x0 > x1 {
		x0, x1 = x1, x0
	}
	for x := x0; x <= x1; x++ {
		lvl.Set(game.Pos{X: x, Y: y}, floor)
	}
}

func carveV(lvl *game.Level, y0, y1, x int, floor *content.TileDef) {
	if y0 > y1 {
		y0, y1 = y1, y0
	}
	for y := y0; y <= y1; y++ {
		lvl.Set(game.Pos{X: x, Y: y}, floor)
	}
}
```

- [ ] **Step 4: Run to verify they pass** — from `go/`: `export GOTOOLCHAIN=local && go test ./internal/gen/` → PASS.

- [ ] **Step 5: Commit**
```bash
git add go/internal/gen/
git commit -m "feat(gen): seeded rooms-and-corridors dungeon generator"
```

---

## Task 4: Wire generated dungeons into `cmd/tsl`

**Files:**
- Modify: `go/cmd/tsl/main.go`
- Modify: `go/cmd/tsl/main_test.go`

- [ ] **Step 1: Replace the smoke test** — replace the entire contents of `go/cmd/tsl/main_test.go` with:
```go
package main

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func testTiles() *content.Content {
	return &content.Content{Tiles: map[string]*content.TileDef{
		"floor":       {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":        {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
		"stairs_down": {ID: "stairs_down", Glyph: ">", Color: content.ColorNormal, Passable: true, Transparent: true},
	}}
}

func TestNewGamePlayable(t *testing.T) {
	c := testTiles()
	g, err := newGame(c, 12345)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if !g.Level.Passable(g.Player) {
		t.Errorf("player start %v is not passable", g.Player)
	}
	moved := false
	for _, d := range []game.Direction{game.DirN, game.DirE, game.DirS, game.DirW} {
		probe := &game.Game{Content: c, Level: g.Level, Player: g.Player}
		if probe.Move(d) {
			moved = true
			break
		}
	}
	if !moved {
		t.Error("player cannot move in any cardinal direction from start")
	}
}

func TestNewGameDeterministic(t *testing.T) {
	c := testTiles()
	a, err := newGame(c, 999)
	if err != nil {
		t.Fatalf("newGame(a): %v", err)
	}
	b, err := newGame(c, 999)
	if err != nil {
		t.Fatalf("newGame(b): %v", err)
	}
	if a.Player != b.Player {
		t.Errorf("same seed gave different starts: %v vs %v", a.Player, b.Player)
	}
}
```

- [ ] **Step 2: Run to verify it fails** — from `go/`: `export GOTOOLCHAIN=local && go test ./cmd/tsl/` → FAIL (`undefined: newGame`).

- [ ] **Step 3: Update main.go** — replace the entire contents of `go/cmd/tsl/main.go` with:
```go
// Command tsl is the console front-end for the Go port of The Slimy Lichmummy.
package main

import (
	"fmt"
	"os"
	"time"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/gen"
	"github.com/c0ze/tsl/internal/rng"
	"github.com/c0ze/tsl/internal/ui"
	tcellui "github.com/c0ze/tsl/internal/ui/tcell"
)

const (
	mapW = 60
	mapH = 24
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, "tsl:", err)
		os.Exit(1)
	}
}

func run() error {
	// content.Load reads ./data relative to the working directory, so tsl must
	// be run from the go/ module root for now. A configurable data path will
	// come with the real game setup in a later plan.
	c, err := content.Load("data")
	if err != nil {
		return err
	}
	g, err := newGame(c, uint32(time.Now().UnixNano()))
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

// newGame builds a fresh, procedurally generated dungeon level seeded by seed.
func newGame(c *content.Content, seed uint32) (*game.Game, error) {
	lvl, start, _, err := gen.Rooms(rng.NewWithSeed(seed), c, mapW, mapH)
	if err != nil {
		return nil, err
	}
	return &game.Game{Content: c, Level: lvl, Player: start}, nil
}
```

- [ ] **Step 4: Run to verify it passes** — from `go/`: `export GOTOOLCHAIN=local && go test ./cmd/tsl/` → PASS.

- [ ] **Step 5: Verify whole module + binary** — from `go/`:
```bash
export GOTOOLCHAIN=local
go build ./... && go test ./... -count=1 && go vet ./...
go build -o /tmp/tsl_p2 ./cmd/tsl && echo "binary OK" && rm -f /tmp/tsl_p2
```
Expected: all packages `ok`, vet clean, binary builds. (Interactive `go run ./cmd/tsl` now shows a generated dungeon with `>` stairs; not runnable headlessly.)

- [ ] **Step 6: Commit**
```bash
git add go/cmd/tsl/
git commit -m "feat(cmd): generate dungeons via gen instead of a static room"
```

---

## Done criteria (Plan 2)

- `internal/rng` reproduces MT19937 bit-for-bit (known-vector test passes).
- `internal/gen.Rooms` produces deterministic, fully-connected levels with a reachable down-stairs and a passable player start (verified over many seeds).
- `cmd/tsl` builds a generated dungeon (no more static room); `go run ./cmd/tsl` shows a walkable, randomized level with `>` stairs.
- `go test ./...`, `go vet ./...`, `go build ./...` all clean from `go/`.
- Dependency direction held: `gen → {game, rng, content}`; `rng` a pure leaf; engine still imports no UI.

## Deferred (later plans, per the design spec)
- Matching the C's specific area generators (Underpass, Laboratory, …) — this generic generator is the incremental first step; faithful area-by-area parity comes once surrounding systems exist.
- Descending stairs to a new level, FOV/visibility, creatures/AI/combat, items, save — Plans 3–6.
