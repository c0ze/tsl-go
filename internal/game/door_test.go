package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/rng"
)

func doorContent() *content.Content {
	return &content.Content{Tiles: map[string]*content.TileDef{
		"floor":       {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":        {ID: "wall", Glyph: "#", Color: content.ColorNormal},
		"door_open":   {ID: "door_open", Glyph: "'", Color: content.ColorBrown, Passable: true, Transparent: true},
		"door_closed": {ID: "door_closed", Glyph: "+", Color: content.ColorBrown, OpensTo: "door_open"},
	}}
}

func doorGame() *Game {
	c := doorContent()
	l := NewLevel(5, 3, c.Tiles["floor"])
	return &Game{Content: c, Level: l, Player: Pos{1, 1}, RNG: rng.NewWithSeed(1), PlayerHP: 10, PlayerMax: 10}
}

func TestBumpOpensDoor(t *testing.T) {
	g := doorGame()
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_closed"])

	g.PlayerStep(DirE) // bump the door east of the player

	if g.Player != (Pos{1, 1}) {
		t.Errorf("player should not step onto a just-opened door, at %v", g.Player)
	}
	if g.Level.At(Pos{2, 1}).Def.ID != "door_open" {
		t.Errorf("bumping a closed door should open it, got %q", g.Level.At(Pos{2, 1}).Def.ID)
	}
	if !g.Level.Passable(Pos{2, 1}) {
		t.Error("an opened door should be passable")
	}
}

func TestMonsterOpensDoor(t *testing.T) {
	g := doorGame()
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_closed"])
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3, Damage: "1d1"}, Pos: Pos{3, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)

	g.worldTick() // rat steps toward the player, blocked by the closed door

	if g.Level.At(Pos{2, 1}).Def.ID != "door_open" {
		t.Errorf("a monster should open a door in its path, got %q", g.Level.At(Pos{2, 1}).Def.ID)
	}
}

func TestBumpWallDoesNotOpen(t *testing.T) {
	g := doorGame()
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["wall"])

	g.PlayerStep(DirE)

	if g.Level.At(Pos{2, 1}).Def.ID != "wall" {
		t.Error("bumping a plain wall should not open anything")
	}
	if g.Player != (Pos{1, 1}) {
		t.Errorf("player should not move into a wall, at %v", g.Player)
	}
}

// A creature that can't walk (a revealed mimic, a pool-bound tentacle) can't
// open a door either: stepping toward the player through one does nothing.
func TestRootedCreatureLeavesDoorShut(t *testing.T) {
	for _, def := range []*content.MonsterDef{
		{ID: "mimic", Name: "mimic", HP: 3, Damage: "1d1", Mimic: true},
		{ID: "tentacle", Name: "tentacle", HP: 3, Damage: "1d1", Swim: true, Permaswim: true},
	} {
		g := doorGame()
		g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_closed"])
		g.Level.Creatures = append(g.Level.Creatures, &Creature{Def: def, Pos: Pos{3, 1}, HP: 3})
		g.worldTick()
		if g.Level.At(Pos{2, 1}).Def.ID != "door_closed" {
			t.Errorf("a %s opened the door", def.ID)
		}
	}
}
