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
