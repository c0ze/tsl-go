package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

// fakeDungeon builds a tiny 2-level graph (a↔b) with a fake builder; each level
// has one portal (at 3,1) to the other. b is a win level iff bWins.
func fakeDungeon(t *testing.T, bWins bool) *Game {
	t.Helper()
	floor := &content.TileDef{ID: "floor", Glyph: ".", Passable: true, Transparent: true}
	defs := map[string]*content.LevelDef{
		"a": {ID: "a", Name: "Level A", W: 5, H: 3, Start: true, Links: []string{"b"}},
		"b": {ID: "b", Name: "Level B", W: 5, H: 3, Links: []string{"a"}, Win: bWins},
	}
	build := func(def *content.LevelDef) (*Level, error) {
		l := NewLevel(def.W, def.H, floor)
		l.Start = Pos{1, 1}
		l.Portals = []Portal{{Pos: Pos{3, 1}, Target: def.Links[0]}}
		return l, nil
	}
	d, err := NewDungeon(defs, "a", build)
	if err != nil {
		t.Fatal(err)
	}
	g := &Game{Content: &content.Content{Levels: defs}, Dungeon: d, Level: d.Current()}
	g.Player = g.Level.Start
	g.Level.entered = true
	return g
}

func TestTravelMovesToLinkedLevel(t *testing.T) {
	g := fakeDungeon(t, false)
	g.Player = Pos{3, 1} // stand on A's portal to B
	g.Travel()
	if g.Dungeon.current != "b" {
		t.Fatalf("current level = %q, want b", g.Dungeon.current)
	}
	if g.Player != g.Level.Start {
		t.Errorf("first arrival at %v, want B.Start %v", g.Player, g.Level.Start)
	}
}

func TestTravelWinsOnWinLevel(t *testing.T) {
	g := fakeDungeon(t, true)
	g.Player = Pos{3, 1}
	g.Travel()
	if !g.Won {
		t.Error("arriving at a win level should win")
	}
}

func TestTravelPersistsLevelState(t *testing.T) {
	g := fakeDungeon(t, false)
	g.Player = Pos{3, 1}
	g.Travel() // → B
	b := g.Level
	b.Creatures = append(b.Creatures, &Creature{Def: &content.MonsterDef{Name: "ghost"}, Pos: Pos{2, 1}, HP: 1})

	g.Player = Pos{3, 1}
	g.Travel() // → back to A
	if g.Dungeon.current != "a" {
		t.Fatalf("expected to be back on A, got %q", g.Dungeon.current)
	}

	g.Player = Pos{3, 1}
	g.Travel() // → B again
	if g.Level != b {
		t.Error("revisiting B should reuse the cached level instance")
	}
	if len(b.Creatures) != 1 {
		t.Errorf("B's creature should persist across visits, got %d", len(b.Creatures))
	}
	if g.Player != b.Return {
		t.Errorf("re-arrival at %v, want B.Return %v", g.Player, b.Return)
	}
}

func TestNewDungeonUnknownStart(t *testing.T) {
	build := func(def *content.LevelDef) (*Level, error) { return NewLevel(5, 3, &content.TileDef{}), nil }
	if _, err := NewDungeon(map[string]*content.LevelDef{}, "missing", build); err == nil {
		t.Fatal("expected error for unknown start level")
	}
}
