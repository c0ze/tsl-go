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
	l1, s1, d1, err := Rooms(rng.NewWithSeed(42), c, 60, 24, 1)
	if err != nil {
		t.Fatal(err)
	}
	l2, s2, d2, err := Rooms(rng.NewWithSeed(42), c, 60, 24, 1)
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
		l, start, down, err := Rooms(rng.NewWithSeed(seed), c, 60, 24, 1)
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

func TestRoomsPlacesItems(t *testing.T) {
	c := testContent()
	c.Items = map[string]*content.ItemDef{
		"potion": {ID: "potion", Name: "potion", Glyph: "!", Color: content.ColorRed, Kind: "potion", Use: "heal", Power: 8},
	}
	l, _, _, err := Rooms(rng.NewWithSeed(3), c, 60, 24, 1)
	if err != nil {
		t.Fatal(err)
	}
	if len(l.Items) == 0 {
		t.Error("expected at least one item placed")
	}
	for _, it := range l.Items {
		if !l.Passable(it.Pos) {
			t.Errorf("item on impassable tile at %v", it.Pos)
		}
	}
}

func TestPlaceItemsSkipsNoSpawn(t *testing.T) {
	c := testContent()
	c.Items = map[string]*content.ItemDef{
		"corpse": {ID: "corpse", Name: "corpse", Glyph: "%", Color: content.ColorBrown, Kind: "food", Use: "eat", Power: 3, NoSpawn: true},
	}
	l, _, _, err := Rooms(rng.NewWithSeed(3), c, 60, 24, 1)
	if err != nil {
		t.Fatal(err)
	}
	if len(l.Items) != 0 {
		t.Errorf("nospawn items must not be placed as floor loot, got %d", len(l.Items))
	}
}

func TestPlaceItemsSeedsWandCharges(t *testing.T) {
	c := testContent()
	c.Items = map[string]*content.ItemDef{
		"wand": {ID: "wand", Name: "wand", Glyph: "/", Color: content.ColorBlue, Kind: "wand", Damage: "2d6", Power: 5},
	}
	for seed := uint32(1); seed <= 20; seed++ {
		lvl, err := LevelFromDef(rng.NewWithSeed(seed), c, &content.LevelDef{ID: "x", Name: "X", W: 60, H: 24, Links: []string{"y"}})
		if err != nil {
			t.Fatal(err)
		}
		for _, it := range lvl.Items {
			if it.Def.Kind == "wand" {
				if it.Charges != 5 {
					t.Fatalf("wand charges = %d, want 5 (from power)", it.Charges)
				}
				return // found a placed wand and verified its charges
			}
		}
	}
	t.Fatal("no wand was placed across 20 seeds")
}

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

func levelDefContent() *content.Content {
	c := testContent()
	c.Monsters = map[string]*content.MonsterDef{
		"ratman": {ID: "ratman", Name: "ratman", Glyph: "r", Color: content.ColorBrown, HP: 3, Attack: 2, Dodge: 1, Damage: "1d2"},
	}
	return c
}

func TestLevelFromDefPlacesPortals(t *testing.T) {
	c := levelDefContent()
	def := &content.LevelDef{ID: "dungeon", Name: "the Dungeon", W: 60, H: 24, Links: []string{"catacombs", "cave"}, Monsters: 5, Spawn: []content.SpawnEntry{{Monster: "ratman", Weight: 1}}}
	lvl, err := LevelFromDef(rng.NewWithSeed(1), c, def)
	if err != nil {
		t.Fatal(err)
	}
	if len(lvl.Portals) != 2 {
		t.Fatalf("expected 2 portals, got %d", len(lvl.Portals))
	}
	targets := map[string]bool{}
	for _, p := range lvl.Portals {
		targets[p.Target] = true
		if !lvl.Passable(p.Pos) {
			t.Errorf("portal at %v should be on a passable tile", p.Pos)
		}
	}
	if !targets["catacombs"] || !targets["cave"] {
		t.Errorf("portals should target catacombs+cave, got %v", targets)
	}
}

func TestLevelFromDefSpawnsOnlyFromTable(t *testing.T) {
	c := levelDefContent()
	c.Monsters["dragon"] = &content.MonsterDef{ID: "dragon", Name: "dragon", Glyph: "D", Color: content.ColorRed, HP: 99, Attack: 9, Dodge: 9, Damage: "1d1"}
	def := &content.LevelDef{ID: "x", Name: "X", W: 60, H: 24, Monsters: 20, Spawn: []content.SpawnEntry{{Monster: "ratman", Weight: 1}}}
	lvl, err := LevelFromDef(rng.NewWithSeed(2), c, def)
	if err != nil {
		t.Fatal(err)
	}
	if len(lvl.Creatures) == 0 {
		t.Fatal("expected some monsters placed")
	}
	for _, m := range lvl.Creatures {
		if m.Def.ID != "ratman" {
			t.Errorf("spawned %q, but only ratman is in the spawn table", m.Def.ID)
		}
	}
}

func TestLevelFromDefPlacesBossAndAltar(t *testing.T) {
	c := levelDefContent()
	c.Monsters["mummylich"] = &content.MonsterDef{ID: "mummylich", Name: "elder mummylich", Glyph: "E", Color: content.ColorMagenta, HP: 40, Attack: 6, Dodge: 3, Damage: "1d8"}
	c.Tiles["altar"] = &content.TileDef{ID: "altar", Glyph: "_", Color: content.ColorCyan, Passable: true, Transparent: true, Win: true}
	def := &content.LevelDef{ID: "chapel", Name: "Chapel", W: 60, H: 24, Links: []string{"x"}, Altar: true, Boss: "mummylich"}
	lvl, err := LevelFromDef(rng.NewWithSeed(1), c, def)
	if err != nil {
		t.Fatal(err)
	}
	bosses := 0
	for _, m := range lvl.Creatures {
		if m.Def.ID == "mummylich" {
			bosses++
		}
	}
	if bosses != 1 {
		t.Errorf("want exactly 1 boss, got %d", bosses)
	}
	found := false
	for y := 0; y < lvl.H && !found; y++ {
		for x := 0; x < lvl.W; x++ {
			if lvl.At(game.Pos{X: x, Y: y}).Def.Win {
				found = true
				break
			}
		}
	}
	if !found {
		t.Error("expected an altar (win) tile on the level")
	}
}

func TestLevelFromDefScattersTraps(t *testing.T) {
	c := levelDefContent()
	c.Tiles["dart_trap"] = &content.TileDef{ID: "dart_trap", Glyph: "^", Color: content.ColorRed, Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
	def := &content.LevelDef{ID: "x", Name: "X", W: 60, H: 24, Links: []string{"y"}, Traps: 5}
	lvl, err := LevelFromDef(rng.NewWithSeed(3), c, def)
	if err != nil {
		t.Fatal(err)
	}
	traps := 0
	for y := 0; y < lvl.H; y++ {
		for x := 0; x < lvl.W; x++ {
			if lvl.At(game.Pos{X: x, Y: y}).Def.Effect != "" {
				traps++
			}
		}
	}
	if traps == 0 {
		t.Error("expected some trap tiles placed")
	}
}

func TestLevelFromDefPlacesDoors(t *testing.T) {
	c := levelDefContent()
	c.Tiles["door_open"] = &content.TileDef{ID: "door_open", Glyph: "'", Color: content.ColorBrown, Passable: true, Transparent: true}
	c.Tiles["door_closed"] = &content.TileDef{ID: "door_closed", Glyph: "+", Color: content.ColorBrown, OpensTo: "door_open"}
	def := &content.LevelDef{ID: "x", Name: "X", W: 60, H: 24, Links: []string{"y"}, Doors: true}
	lvl, err := LevelFromDef(rng.NewWithSeed(1), c, def)
	if err != nil {
		t.Fatal(err)
	}
	doors := 0
	for y := 0; y < lvl.H; y++ {
		for x := 0; x < lvl.W; x++ {
			p := game.Pos{X: x, Y: y}
			if lvl.At(p).Def.ID != "door_closed" {
				continue
			}
			doors++
			// A door sits on a 1-wide passage: exactly one opposite axis open.
			up := lvl.Passable(game.Pos{X: x, Y: y - 1})
			down := lvl.Passable(game.Pos{X: x, Y: y + 1})
			left := lvl.Passable(game.Pos{X: x - 1, Y: y})
			right := lvl.Passable(game.Pos{X: x + 1, Y: y})
			if !((up && down && !left && !right) || (left && right && !up && !down)) {
				t.Errorf("door at %v is not on a clean doorway (u%v d%v l%v r%v)", p, up, down, left, right)
			}
		}
	}
	if doors == 0 {
		t.Error("expected at least one door placed at a doorway")
	}
}

func TestLevelFromDefCarriesDarkFlag(t *testing.T) {
	c := levelDefContent()
	def := &content.LevelDef{ID: "x", Name: "X", W: 60, H: 24, Links: []string{"y"}, Dark: true}
	lvl, err := LevelFromDef(rng.NewWithSeed(1), c, def)
	if err != nil {
		t.Fatal(err)
	}
	if !lvl.Dark {
		t.Error("a dark level def should produce a dark level")
	}
}

func TestLevelFromDefDeterministic(t *testing.T) {
	c := levelDefContent()
	def := &content.LevelDef{ID: "x", Name: "X", W: 60, H: 24, Links: []string{"y"}, Monsters: 5, Spawn: []content.SpawnEntry{{Monster: "ratman", Weight: 1}}}
	a, _ := LevelFromDef(rng.NewWithSeed(7), c, def)
	b, _ := LevelFromDef(rng.NewWithSeed(7), c, def)
	if glyphGrid(a) != glyphGrid(b) {
		t.Error("same seed should yield the same level layout")
	}
}

func TestRoomsPlacesMonsters(t *testing.T) {
	c := testContent()
	c.Monsters = map[string]*content.MonsterDef{
		"rat": {ID: "rat", Name: "rat", Glyph: "r", Color: content.ColorBrown, HP: 3, Attack: 2, Dodge: 1, Damage: "1d2"},
	}
	l, _, _, err := Rooms(rng.NewWithSeed(7), c, 60, 24, 1)
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
