package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/rng"
)

// fakeDungeon builds a tiny 2-level graph (a↔b) with a fake builder; each level
// has one portal (at 3,1) to the other.
func fakeDungeon(t *testing.T) *Game {
	t.Helper()
	floor := &content.TileDef{ID: "floor", Glyph: ".", Passable: true, Transparent: true}
	defs := map[string]*content.LevelDef{
		"a": {ID: "a", Name: "Level A", W: 5, H: 3, Start: true, Links: []string{"b"}},
		"b": {ID: "b", Name: "Level B", W: 5, H: 3, Links: []string{"a"}},
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
	g := &Game{Content: &content.Content{Levels: defs}, Dungeon: d, Level: d.Current(), RNG: rng.NewWithSeed(1), PlayerHP: 20, PlayerMax: 20}
	g.Player = g.Level.Start
	g.Level.entered = true
	return g
}

func TestTravelMovesToLinkedLevel(t *testing.T) {
	g := fakeDungeon(t)
	g.Player = Pos{3, 1} // stand on A's portal to B
	g.Travel()
	if g.Dungeon.current != "b" {
		t.Fatalf("current level = %q, want b", g.Dungeon.current)
	}
	if g.Player != (Pos{3, 1}) {
		t.Errorf("arrival at %v, want B's stairs back to A at {3 1} (C traverse_branch)", g.Player)
	}
}

func TestTravelPersistsLevelState(t *testing.T) {
	g := fakeDungeon(t)
	g.Player = Pos{3, 1}
	g.Travel() // → B
	b := g.Level
	b.Creatures = append(b.Creatures, &Creature{Def: &content.MonsterDef{Name: "ghost"}, Pos: Pos{0, 0}, HP: 1}) // away from the stairs

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
	if g.Player != (Pos{3, 1}) {
		t.Errorf("re-arrival at %v, want B's stairs {3 1}", g.Player)
	}
}

// Creatures beside the stairs follow the player through them (C
// move_everyone) — an ally and a pursuer alike — but a disguised mimic stays,
// and taking the stairs costs the turn.
func TestTravelBringsAdjacentCreatures(t *testing.T) {
	g := fakeDungeon(t)
	a := g.Level
	g.Player = Pos{3, 1}
	g.EPMax = 5 // below max, so each passing turn ticks EP regeneration
	imp := &Creature{Def: &content.MonsterDef{ID: "imp", Name: "imp"}, Pos: Pos{2, 1}, HP: 5, Ally: true}
	mimic := &Creature{Def: &content.MonsterDef{ID: "mimic", Name: "mimic"}, Pos: Pos{4, 1}, HP: 5, Disguised: true}
	far := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat"}, Pos: Pos{0, 0}, HP: 5}
	a.Creatures = append(a.Creatures, imp, mimic, far)
	g.Travel()
	if len(g.Level.Creatures) != 1 || g.Level.Creatures[0] != imp {
		t.Fatalf("only the adjacent imp should follow, B has %v", g.Level.Creatures)
	}
	if chebyshev(imp.Pos, Pos{3, 1}) > 2 || imp.Pos == g.Player {
		t.Errorf("follower landed at %v, player at %v", imp.Pos, g.Player)
	}
	if len(a.Creatures) != 2 {
		t.Errorf("the mimic and the distant rat stay behind, A has %d", len(a.Creatures))
	}
	if g.epTurn == 0 {
		t.Error("climbing the stairs should cost the turn")
	}
}

func TestNewDungeonUnknownStart(t *testing.T) {
	build := func(def *content.LevelDef) (*Level, error) { return NewLevel(5, 3, &content.TileDef{}), nil }
	if _, err := NewDungeon(map[string]*content.LevelDef{}, "missing", build); err == nil {
		t.Fatal("expected error for unknown start level")
	}
}

// Arriving on stairs a creature already occupies puts the player beside it,
// never on its tile.
func TestTravelArrivalAvoidsOccupiedStairs(t *testing.T) {
	g := fakeDungeon(t)
	g.Player = Pos{3, 1}
	g.Travel() // generate B
	b := g.Level
	g.Player = Pos{3, 1}
	g.Travel() // back to A
	squatter := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat"}, Pos: Pos{3, 1}, HP: 5}
	b.Creatures = append(b.Creatures, squatter)
	g.Player = Pos{3, 1}
	g.Travel() // onto B's occupied stairs
	if g.Player == squatter.Pos {
		t.Fatalf("player shares the squatter's tile at %v", g.Player)
	}
	if chebyshev(g.Player, Pos{3, 1}) != 1 {
		t.Errorf("player should land beside the stairs, at %v", g.Player)
	}
}

// With the stairs occupied and every nearby tile taken, the arrival still
// finds a free tile somewhere (C find_random_free_spot), never the squatter's.
func TestTravelArrivalFallsBackToAnyFreeTile(t *testing.T) {
	g := fakeDungeon(t)
	g.Player = Pos{3, 1}
	g.Travel() // generate B
	b := g.Level
	g.Player = Pos{3, 1}
	g.Travel() // back to A
	for y := 0; y < b.H; y++ {
		for x := 0; x < b.W; x++ {
			if x != 0 || y != 0 { // leave one far corner free
				b.Creatures = append(b.Creatures, &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat"}, Pos: Pos{x, y}, HP: 5})
			}
		}
	}
	g.Player = Pos{3, 1}
	g.Travel()
	if g.Player != (Pos{0, 0}) {
		t.Errorf("player should land on the only free tile {0 0}, at %v", g.Player)
	}
}
