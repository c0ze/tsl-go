# TSL Go Port — M1 Plan 4: Creatures, AI, Combat, Message Log

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or executing-plans. Steps use checkbox (`- [ ]`).

**Goal:** Monsters populate the dungeon, take turns, and hunt the player; the player bump-attacks them; both sides have HP and can die; a message log reports what happens.

**Architecture:** `content` gains `MonsterDef` + optional `monsters.toml`. `rng` gains the C roll system (`Roll`, `Chance`=roll_xn, `RollSpec`). `game` gains a `Creature` type (monsters), a per-game `RNG`, player HP, a message log, monster turns/AI, and stat-based melee combat. `gen` places monsters. `ui`/`tcell` render visible monsters and a message line. The engine still imports no UI.

**Combat fidelity:** hit chance uses the C's `roll_xn(attack, dodge)` (`rng.Chance`); damage is a dice roll (`rng.RollSpec`). This is the faithful melee core; weapon damage-sequences/wounds/ranged (the rest of `tsl-0.40/common/combat.c`) arrive with items in a later plan.

**Player model note:** the player stays a `Pos` + HP/stat fields on `Game` (monsters are `Creature`s); combat is stat-based so it works across both without a unified type. Unifying player-as-`Creature` is a deferred refactor.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`). No new deps.

## Working dir & environment
- Module in `go/`; `export GOTOOLCHAIN=local`. Branch `plan4-combat`. No `go mod tidy`/`go get`.

---

## Task 1: Monster content (`content`)

**Files:** Modify `internal/content/content.go`; create `data/monsters.toml`; modify `internal/content/content_test.go`

- [ ] **Step 1** — create `data/monsters.toml`:
```toml
# Monster definitions. [monster.<id>] → MonsterDef keyed by <id>.
[monster.rat]
name = "rat"
glyph = "r"
color = "brown"
hp = 3
attack = 2
dodge = 1
damage = "1d2"

[monster.ghoul]
name = "ghoul"
glyph = "g"
color = "green"
hp = 8
attack = 4
dodge = 2
damage = "1d4"
```

- [ ] **Step 2** — append tests to `internal/content/content_test.go`:
```go
func TestLoadMonsters(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "tiles.toml"), []byte(`
[tile.floor]
glyph = "."
color = "normal"
passable = true
transparent = true
`), 0o644); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, "monsters.toml"), []byte(`
[monster.rat]
name = "rat"
glyph = "r"
color = "brown"
hp = 3
attack = 2
dodge = 1
damage = "1d2"
`), 0o644); err != nil {
		t.Fatal(err)
	}
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	rat, ok := c.Monsters["rat"]
	if !ok {
		t.Fatal("rat monster missing")
	}
	if rat.ID != "rat" || rat.HP != 3 || rat.Rune() != 'r' {
		t.Errorf("unexpected rat def: %+v", rat)
	}
}

func TestLoadWithoutMonstersFileIsOK(t *testing.T) {
	dir := writeTiles(t, `
[tile.floor]
glyph = "."
color = "normal"
passable = true
transparent = true
`)
	c, err := Load(dir)
	if err != nil {
		t.Fatalf("Load: %v", err)
	}
	if len(c.Monsters) != 0 {
		t.Errorf("expected no monsters, got %d", len(c.Monsters))
	}
}
```

- [ ] **Step 3** — in `internal/content/content.go`: add `"os"` to imports; add the `MonsterDef` type, the `Monsters` map on `Content`, and optional monster loading.

Add after `TileDef`:
```go
// MonsterDef defines a kind of monster.
type MonsterDef struct {
	ID     string `toml:"-"`
	Name   string `toml:"name"`
	Glyph  string `toml:"glyph"`
	Color  Color  `toml:"color"`
	HP     int    `toml:"hp"`
	Attack int    `toml:"attack"`
	Dodge  int    `toml:"dodge"`
	Damage string `toml:"damage"` // dice spec, e.g. "1d4"
}

// Rune returns the monster's glyph as a rune.
func (m *MonsterDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(m.Glyph)
	return r
}
```

Change `Content` to:
```go
// Content is the fully-loaded, validated game content.
type Content struct {
	Tiles    map[string]*TileDef
	Monsters map[string]*MonsterDef
}
```

Add the monsters file struct near `tilesFile`:
```go
type monstersFile struct {
	Monster map[string]*MonsterDef `toml:"monster"`
}
```

In `Load`, initialise the map and load monsters optionally. After building `c.Tiles` (before `return c, nil`), insert:
```go
	c.Monsters = map[string]*MonsterDef{}
	mpath := filepath.Join(dir, "monsters.toml")
	if _, err := os.Stat(mpath); err == nil {
		var mf monstersFile
		if _, err := toml.DecodeFile(mpath, &mf); err != nil {
			return nil, fmt.Errorf("loading %s: %w", mpath, err)
		}
		for id, def := range mf.Monster {
			def.ID = id
			if err := validateMonster(def); err != nil {
				return nil, fmt.Errorf("monster %q in %s: %w", id, mpath, err)
			}
			c.Monsters[id] = def
		}
	}
```
And the `Content` literal at the top of `Load` should also set `Monsters: map[string]*MonsterDef{}` (or leave the assignment above as the initializer). Add the validator:
```go
func validateMonster(m *MonsterDef) error {
	if utf8.RuneCountInString(m.Glyph) != 1 {
		return fmt.Errorf("glyph must be exactly one character, got %q", m.Glyph)
	}
	if !validColors[m.Color] {
		return fmt.Errorf("invalid color %q", m.Color)
	}
	if m.HP < 1 {
		return fmt.Errorf("hp must be >= 1, got %d", m.HP)
	}
	return nil
}
```

- [ ] **Step 4** — `export GOTOOLCHAIN=local && go test ./internal/content/` → PASS. **Commit:**
```bash
git add internal/content/ data/monsters.toml
git commit -m "feat(content): MonsterDef + optional monsters.toml loading"
```

---

## Task 2: Dice & odds in `rng`

**Files:** Create `internal/rng/dice.go`, `internal/rng/dice_test.go`

- [ ] **Step 1** — `internal/rng/dice_test.go`:
```go
package rng

import "testing"

func TestRollInRange(t *testing.T) {
	g := NewWithSeed(1)
	for i := 0; i < 1000; i++ {
		v := g.Roll(2, 6) // 2..12
		if v < 2 || v > 12 {
			t.Fatalf("Roll(2,6) = %d out of [2,12]", v)
		}
	}
}

func TestChanceBounds(t *testing.T) {
	g := NewWithSeed(1)
	// x huge vs n=1 → almost always true; n huge vs x=1 → almost always false.
	hi := 0
	for i := 0; i < 1000; i++ {
		if g.Chance(100, 1) {
			hi++
		}
	}
	if hi < 900 {
		t.Errorf("Chance(100,1) true only %d/1000, expected most", hi)
	}
	lo := 0
	for i := 0; i < 1000; i++ {
		if g.Chance(1, 100) {
			lo++
		}
	}
	if lo > 100 {
		t.Errorf("Chance(1,100) true %d/1000, expected few", lo)
	}
}

func TestRollSpec(t *testing.T) {
	g := NewWithSeed(1)
	for i := 0; i < 1000; i++ {
		if v := g.RollSpec("1d4"); v < 1 || v > 4 {
			t.Fatalf("RollSpec(1d4) = %d out of [1,4]", v)
		}
		if v := g.RollSpec("2d3+1"); v < 3 || v > 7 {
			t.Fatalf("RollSpec(2d3+1) = %d out of [3,7]", v)
		}
	}
	if g.RollSpec("garbage") != 0 {
		t.Error("RollSpec on malformed spec should return 0")
	}
}
```

- [ ] **Step 2** — Run → FAIL. **Step 3** — `internal/rng/dice.go`:
```go
package rng

import "strconv"

// Roll returns the sum of n dice each in [1, sides] (the C roll()).
func (g *MT) Roll(n, sides int) int {
	if sides < 1 {
		return n
	}
	sum := 0
	for i := 0; i < n; i++ {
		sum += 1 + g.Intn(sides)
	}
	return sum
}

// Chance returns true with probability x/(x+n) (the C roll_xn). Non-positive x
// or n are clamped to >=1 by shifting the other up, matching the original.
func (g *MT) Chance(x, n int) bool {
	if x < 1 {
		n += 1 - x
		x = 1
	}
	if n < 1 {
		x += 1 - n
		n = 1
	}
	return g.Intn(x+n) < x
}

// RollSpec rolls a dice spec "NdS" or "NdS+M" / "NdS-M" (the C sroll). It
// returns 0 on a malformed spec.
func (g *MT) RollSpec(spec string) int {
	d := indexByte(spec, 'd')
	if d <= 0 || d >= len(spec)-1 {
		return 0
	}
	n, err := strconv.Atoi(spec[:d])
	if err != nil {
		return 0
	}
	rest := spec[d+1:]
	mod := 0
	if i := indexAnySign(rest); i >= 0 {
		m, err := strconv.Atoi(rest[i:])
		if err != nil {
			return 0
		}
		mod = m
		rest = rest[:i]
	}
	sides, err := strconv.Atoi(rest)
	if err != nil {
		return 0
	}
	return g.Roll(n, sides) + mod
}

func indexByte(s string, b byte) int {
	for i := 0; i < len(s); i++ {
		if s[i] == b {
			return i
		}
	}
	return -1
}

func indexAnySign(s string) int {
	for i := 1; i < len(s); i++ { // start at 1: a leading sign isn't a modifier
		if s[i] == '+' || s[i] == '-' {
			return i
		}
	}
	return -1
}
```

- [ ] **Step 4** — `go test ./internal/rng/` → PASS. **Commit:**
```bash
git add internal/rng/dice.go internal/rng/dice_test.go
git commit -m "feat(rng): dice rolls and roll_xn odds (Roll/Chance/RollSpec)"
```

---

## Task 3: Creatures on the level (`game`)

**Files:** Create `internal/game/creature.go`, `internal/game/creature_test.go`

- [ ] **Step 1** — `internal/game/creature_test.go`:
```go
package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestCreatureAtAndRemove(t *testing.T) {
	c := testContent()
	l := NewLevel(5, 5, c.Tiles["floor"])
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 3}, Pos: Pos{2, 2}, HP: 3}
	l.Creatures = append(l.Creatures, rat)
	if got := l.CreatureAt(Pos{2, 2}); got != rat {
		t.Fatalf("CreatureAt(2,2) = %v, want rat", got)
	}
	if got := l.CreatureAt(Pos{0, 0}); got != nil {
		t.Errorf("CreatureAt(0,0) = %v, want nil", got)
	}
	l.RemoveCreature(rat)
	if got := l.CreatureAt(Pos{2, 2}); got != nil {
		t.Error("rat should be gone after RemoveCreature")
	}
}
```

- [ ] **Step 2** — Run → FAIL. **Step 3** — `internal/game/creature.go`:
```go
package game

import "github.com/c0ze/tsl/internal/content"

// Faction marks whose side a creature is on.
type Faction int

const (
	FactionEnemy Faction = iota
	FactionPlayer
)

// Creature is a monster on the level (the player is modelled separately on Game
// for now). HP is current hit points; Def carries its stats and glyph.
type Creature struct {
	Def     *content.MonsterDef
	Pos     Pos
	HP      int
	Faction Faction
}

// CreatureAt returns the creature standing on p, or nil.
func (l *Level) CreatureAt(p Pos) *Creature {
	for _, cr := range l.Creatures {
		if cr.Pos == p {
			return cr
		}
	}
	return nil
}

// RemoveCreature removes cr from the level (no-op if absent).
func (l *Level) RemoveCreature(cr *Creature) {
	for i, x := range l.Creatures {
		if x == cr {
			l.Creatures = append(l.Creatures[:i], l.Creatures[i+1:]...)
			return
		}
	}
}
```

Also add the `Creatures` field to `Level` in `game.go` — change the `Level` struct to:
```go
// Level is a rectangular grid of tiles stored row-major.
type Level struct {
	W, H      int
	tiles     []Tile
	Creatures []*Creature
}
```

- [ ] **Step 4** — `go test ./internal/game/` → PASS. **Commit:**
```bash
git add internal/game/creature.go internal/game/creature_test.go internal/game/game.go
git commit -m "feat(game): Creature type and level creature list"
```

---

## Task 4: Turns, AI, and melee combat (`game`)

**Files:** Modify `internal/game/game.go` (Game fields); create `internal/game/combat.go`, `internal/game/combat_test.go`

- [ ] **Step 1** — extend the `Game` struct in `internal/game/game.go` to:
```go
// Game is the whole game state.
type Game struct {
	Content   *content.Content
	Level     *Level
	Player    Pos
	RNG       *rng.MT
	PlayerHP  int
	PlayerMax int
	Messages  []string
	Dead      bool
}
```
and add the import `"github.com/c0ze/tsl/internal/rng"` to `game.go`.

- [ ] **Step 2** — `internal/game/combat_test.go`:
```go
package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

func combatGame() *Game {
	c := testContent()
	l := NewLevel(10, 3, c.Tiles["floor"])
	return &Game{
		Content: c, Level: l, Player: Pos{1, 1},
		RNG: rng.NewWithSeed(1), PlayerHP: 20, PlayerMax: 20,
	}
}

func TestPlayerBumpKillsMonster(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 1, Attack: 1, Dodge: 0, Damage: "1d1"}, Pos: Pos{2, 1}, HP: 1}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	// Bump east into the rat repeatedly until it dies (HP=1, so first solid hit).
	for i := 0; i < 50 && g.Level.CreatureAt(Pos{2, 1}) != nil; i++ {
		g.PlayerStep(DirE)
	}
	if g.Level.CreatureAt(Pos{2, 1}) != nil {
		t.Fatal("rat should be dead after repeated bumps")
	}
	if g.Player != (Pos{1, 1}) {
		t.Errorf("player should not have moved onto the rat's tile while attacking; at %v", g.Player)
	}
	if len(g.Messages) == 0 {
		t.Error("expected combat messages")
	}
}

func TestMonsterAttacksAndCanKillPlayer(t *testing.T) {
	g := combatGame()
	g.PlayerHP = 1
	ogre := &Creature{Def: &content.MonsterDef{ID: "ogre", Name: "ogre", Glyph: "O", HP: 99, Attack: 100, Dodge: 0, Damage: "5d6"}, Pos: Pos{2, 1}, HP: 99}
	g.Level.Creatures = append(g.Level.Creatures, ogre)
	// Player waits in place (steps into a wall-less open tile west and back is
	// unnecessary); just run monster turns directly.
	for i := 0; i < 50 && !g.Dead; i++ {
		g.monstersAct()
	}
	if !g.Dead {
		t.Fatal("player should be dead after the ogre attacks")
	}
}

func TestMonsterMovesTowardPlayer(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 3, Attack: 1, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	before := rat.Pos.X
	g.monstersAct()
	if rat.Pos.X >= before {
		t.Errorf("rat at x=%d should have stepped toward the player (x decreasing), was %d", rat.Pos.X, before)
	}
}
```

- [ ] **Step 3** — `internal/game/combat.go`:
```go
package game

import "fmt"

// Player melee stats (constant until the player becomes attribute-driven).
const (
	playerAttack = 5
	playerDodge  = 3
	playerDamage = "1d4"
)

const senseRange = 8 // how close a monster must be to notice the player

// log appends a message to the game log.
func (g *Game) log(format string, args ...any) {
	g.Messages = append(g.Messages, fmt.Sprintf(format, args...))
}

// PlayerStep performs the player's turn: bump-attack a monster in direction d,
// otherwise move. Then every monster acts.
func (g *Game) PlayerStep(d Direction) {
	if g.Dead {
		return
	}
	dx, dy := d.Delta()
	dst := Pos{g.Player.X + dx, g.Player.Y + dy}
	acted := false
	if m := g.Level.CreatureAt(dst); m != nil {
		g.playerAttacks(m)
		acted = true
	} else if g.Move(d) {
		acted = true
	}
	if acted { // a blocked move into a wall doesn't pass the turn
		g.monstersAct()
	}
}

func (g *Game) playerAttacks(m *Creature) {
	if !g.RNG.Chance(playerAttack, m.Def.Dodge) {
		g.log("You miss the %s.", m.Def.Name)
		return
	}
	dmg := g.RNG.RollSpec(playerDamage)
	m.HP -= dmg
	g.log("You hit the %s for %d.", m.Def.Name, dmg)
	if m.HP <= 0 {
		g.log("The %s dies.", m.Def.Name)
		g.Level.RemoveCreature(m)
	}
}

func (g *Game) monsterAttacks(m *Creature) {
	if !g.RNG.Chance(m.Def.Attack, playerDodge) {
		g.log("The %s misses you.", m.Def.Name)
		return
	}
	dmg := g.RNG.RollSpec(m.Def.Damage)
	g.PlayerHP -= dmg
	g.log("The %s hits you for %d.", m.Def.Name, dmg)
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.log("You die.")
	}
}

// monstersAct gives each living monster a turn: attack the player if adjacent,
// else step toward the player if within sense range, else idle.
func (g *Game) monstersAct() {
	// snapshot to avoid mutation surprises when creatures are removed
	snapshot := make([]*Creature, len(g.Level.Creatures))
	copy(snapshot, g.Level.Creatures)
	for _, m := range snapshot {
		if g.Dead {
			return
		}
		if g.Level.CreatureAt(m.Pos) != m {
			continue // already removed this turn
		}
		if chebyshev(m.Pos, g.Player) == 1 {
			g.monsterAttacks(m)
			continue
		}
		if chebyshev(m.Pos, g.Player) <= senseRange {
			g.stepToward(m, g.Player)
		}
	}
}

func (g *Game) stepToward(m *Creature, target Pos) {
	step := func(a, b int) int {
		switch {
		case b > a:
			return 1
		case b < a:
			return -1
		default:
			return 0
		}
	}
	dst := Pos{m.Pos.X + step(m.Pos.X, target.X), m.Pos.Y + step(m.Pos.Y, target.Y)}
	if dst == g.Player || !g.Level.Passable(dst) || g.Level.CreatureAt(dst) != nil {
		return
	}
	m.Pos = dst
}

func chebyshev(a, b Pos) int {
	dx, dy := a.X-b.X, a.Y-b.Y
	if dx < 0 {
		dx = -dx
	}
	if dy < 0 {
		dy = -dy
	}
	if dx > dy {
		return dx
	}
	return dy
}
```
(`combat.go` accesses monster stats through `Creature.Def`, so it does not import `content` directly — only `fmt`.)

- [ ] **Step 4** — `go test ./internal/game/` → PASS. **Commit:**
```bash
git add internal/game/game.go internal/game/combat.go internal/game/combat_test.go
git commit -m "feat(game): monster turns, hunt AI, and bump-to-attack melee"
```

---

## Task 5: Place monsters during generation (`gen`)

**Files:** Modify `internal/gen/gen.go`, `internal/gen/gen_test.go`

- [ ] **Step 1** — append a test to `internal/gen/gen_test.go`:
```go
func TestRoomsPlacesMonsters(t *testing.T) {
	c := testContent()
	c.Monsters = map[string]*content.MonsterDef{
		"rat": {ID: "rat", Name: "rat", Glyph: "r", Color: content.ColorBrown, HP: 3, Attack: 2, Dodge: 1, Damage: "1d2"},
	}
	l, _, _, err := Rooms(rng.NewWithSeed(7), c, 60, 24)
	if err != nil {
		t.Fatal(err)
	}
	if len(l.Creatures) == 0 {
		t.Error("expected at least one monster placed")
	}
	for _, m := range l.Creatures {
		if !l.Passable(m.Pos) {
			t.Errorf("monster placed on impassable tile at %v", m.Pos)
		}
		if m.HP != m.Def.HP {
			t.Errorf("monster HP %d != def HP %d", m.HP, m.Def.HP)
		}
	}
}
```
(Note: the existing `testContent()` in gen_test has no Monsters; this test sets them. Other gen tests with empty Monsters must still pass — placement is skipped when no monsters exist.)

- [ ] **Step 2** — Run → FAIL. **Step 3** — in `internal/gen/gen.go`, after the rooms loop and after computing `start`/`down` (before `return`), add monster placement, and add the helper. Insert before `return lvl, start, down, nil`:
```go
	placeMonsters(r, c, lvl, rooms, start)
```
Add at end of file:
```go
// placeMonsters drops 0-2 monsters into each room except the starting room.
func placeMonsters(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, start game.Pos) {
	ids := make([]string, 0, len(c.Monsters))
	for id := range c.Monsters {
		ids = append(ids, id)
	}
	if len(ids) == 0 {
		return
	}
	sort.Strings(ids) // deterministic ordering for a given seed
	for i, room := range rooms {
		if i == 0 {
			continue // keep the starting room clear
		}
		n := r.Intn(3) // 0..2
		for k := 0; k < n; k++ {
			pos := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
			if pos == start || !lvl.Passable(pos) || lvl.CreatureAt(pos) != nil {
				continue
			}
			def := c.Monsters[ids[r.Intn(len(ids))]]
			lvl.Creatures = append(lvl.Creatures, &game.Creature{Def: def, Pos: pos, HP: def.HP})
		}
	}
}
```
Add `"sort"` to the imports in `gen.go`.

- [ ] **Step 4** — `go test ./internal/gen/` → PASS. **Commit:**
```bash
git add internal/gen/
git commit -m "feat(gen): place monsters in generated rooms"
```

---

## Task 6: Render monsters + messages (`ui`, `ui/tcell`)

**Files:** Modify `internal/ui/ui.go`, `internal/ui/ui_test.go`, `internal/ui/tcell/screen.go`

- [ ] **Step 1** — append a test to `internal/ui/ui_test.go`:
```go
func TestBuildViewShowsVisibleMonsterAndMessages(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.UpdateFOV()
	g.Level.Creatures = append(g.Level.Creatures, &game.Creature{
		Def: &content.MonsterDef{ID: "rat", Glyph: "r", Color: content.ColorBrown, HP: 3}, Pos: game.Pos{X: 3, Y: 1}, HP: 3,
	})
	g.Messages = []string{"You hit the rat for 2.", "The rat dies."}
	v := BuildView(g)
	if c := v.At(3, 1); c.Glyph != 'r' {
		t.Errorf("visible monster glyph = %q, want 'r'", c.Glyph)
	}
	if len(v.Messages) == 0 || v.Messages[len(v.Messages)-1] != "The rat dies." {
		t.Errorf("view should carry recent messages, got %v", v.Messages)
	}
}
```

- [ ] **Step 2** — Run → FAIL. **Step 3** — in `internal/ui/ui.go`:

(a) add `Messages` to `View`:
```go
// View is a read-only snapshot the front-end draws.
type View struct {
	W, H     int
	Cells    []Cell // len W*H, row-major
	Messages []string
}
```

(b) in `BuildView`, after drawing tiles and before drawing the player, draw visible monsters; and set `v.Messages`. Replace the player-draw tail of `BuildView` with:
```go
	for _, m := range l.Creatures {
		if l.At(m.Pos).Visible {
			*v.At(m.Pos.X, m.Pos.Y) = Cell{Glyph: m.Def.Rune(), Color: m.Def.Color}
		}
	}
	*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: PlayerGlyph, Color: PlayerColor}
	v.Messages = lastN(g.Messages, 4)
	return v
}

// lastN returns up to the last n elements of s.
func lastN(s []string, n int) []string {
	if len(s) <= n {
		return s
	}
	return s[len(s)-n:]
}
```

(c) make `Run` drive player turns through `PlayerStep` and stop on death:
```go
// Run is the core game loop: recompute FOV, render, get an action, apply it.
func Run(g *game.Game, p Prompter, r Renderer) error {
	for {
		g.UpdateFOV()
		r.Render(BuildView(g))
		if g.Dead {
			return nil
		}
		a, err := p.NextAction()
		if err != nil {
			return err
		}
		switch a.Kind {
		case ActQuit:
			return nil
		case ActMove:
			g.PlayerStep(a.Dir)
		}
	}
}
```

- [ ] **Step 4** — in `internal/ui/tcell/screen.go`, render the message lines under the map. Replace `Render` with:
```go
// Render draws the view (map then message lines) and flushes it.
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
	for i, msg := range v.Messages {
		drawString(sc.s, 0, v.H+i, msg)
	}
	sc.s.Show()
}

func drawString(s tc.Screen, x, y int, str string) {
	for i, r := range str {
		s.SetContent(x+i, y, r, nil, tc.StyleDefault)
	}
}
```

- [ ] **Step 5** — Verify whole module from `go/`:
```bash
export GOTOOLCHAIN=local
gofmt -l .
go build ./... && go test ./... -count=1 && go vet ./...
go build -o /tmp/tsl_p4 ./cmd/tsl && echo "binary OK" && rm -f /tmp/tsl_p4
```
Expected: gofmt clean, all packages `ok`, vet clean, binary builds.

- [ ] **Step 6** — Commit:
```bash
git add internal/ui/
git commit -m "feat(ui): render visible monsters and a message log line"
```

---

## Task 7: Wire HP + RNG into the game (`cmd/tsl`)

**Files:** Modify `cmd/tsl/main.go`, `cmd/tsl/main_test.go`

- [ ] **Step 1** — update `cmd/tsl/main_test.go`'s `newGame` expectations: the `testTiles()` content has no monsters, so `newGame` still works; assert the game is initialised with HP and an RNG. Append:
```go
func TestNewGameHasPlayerHPAndRNG(t *testing.T) {
	c := testTiles()
	g, err := newGame(c, 1)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if g.PlayerHP <= 0 || g.PlayerMax <= 0 {
		t.Errorf("player HP not initialised: hp=%d max=%d", g.PlayerHP, g.PlayerMax)
	}
	if g.RNG == nil {
		t.Error("game RNG not set")
	}
}
```

- [ ] **Step 2** — update `newGame` in `cmd/tsl/main.go` to seed an RNG, share it with generation, and set HP:
```go
// newGame builds a fresh, procedurally generated dungeon level seeded by seed.
func newGame(c *content.Content, seed uint32) (*game.Game, error) {
	r := rng.NewWithSeed(seed)
	lvl, start, _, err := gen.Rooms(r, c, mapW, mapH)
	if err != nil {
		return nil, err
	}
	const startHP = 20
	return &game.Game{
		Content:   c,
		Level:     lvl,
		Player:    start,
		RNG:       r,
		PlayerHP:  startHP,
		PlayerMax: startHP,
	}, nil
}
```
(`gen.Rooms` already takes `r`; the same generator stream continues into combat — deterministic for a seed.)

- [ ] **Step 3** — Whole module green from `go/`: `export GOTOOLCHAIN=local && go build ./... && go test ./... -count=1 && go vet ./...`. **Commit:**
```bash
git add cmd/tsl/
git commit -m "feat(cmd): seed a shared RNG and give the player HP"
```

---

## Done criteria (Plan 4)
- Monsters load from `data/monsters.toml`; `gen` places them in rooms (not the start room).
- Monsters take turns: hunt the player within sense range, attack when adjacent.
- Player bump-attacks adjacent monsters; hits use `roll_xn` odds, damage uses dice; HP reaches 0 → death (monster removed / game over).
- A message log reports combat; the tcell front-end shows recent messages under the map and renders visible monsters.
- `go run ./cmd/tsl`: explore, get hunted, fight, win the exchange or die.
- `go test ./...`, `go vet`, gofmt all clean.

## Deferred
- Speed/energy scheduler (monsters currently act once per player turn); sleep/wake state machine; LOS-based detection.
- Weapon damage-sequences, wounds, ranged, gore — the rest of `tsl-0.40/common/combat.c`, with items.
- Player-as-`Creature` unification; status bar (HP display) beyond the message line.
