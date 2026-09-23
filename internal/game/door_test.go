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

// A creature whose step is refused still tries a closed door in its way —
// even a revealed mimic or a pool-bound tentacle (C pursue: move_creature
// fails, then open_door), since neither lacks the knack for doors.
func TestRootedCreatureStillOpensDoor(t *testing.T) {
	for _, def := range []*content.MonsterDef{
		{ID: "mimic", Name: "mimic", HP: 3, Damage: "1d1", Mimic: true},
		{ID: "tentacle", Name: "tentacle", HP: 3, Damage: "1d1", Swim: true, Permaswim: true},
	} {
		g := doorGame()
		g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_closed"])
		g.Level.Creatures = append(g.Level.Creatures, &Creature{Def: def, Pos: Pos{3, 1}, HP: 3})
		g.worldTick()
		if g.Level.At(Pos{2, 1}).Def.ID != "door_open" {
			t.Errorf("a %s should open the door as in the C", def.ID)
		}
	}
}

// A creature without the knack for doors (C attr_p_open_doors) rattles a
// closed one instead of opening it, and is sometimes heard doing so.
func TestNoDoorsCreatureCannotOpen(t *testing.T) {
	heard := false
	for seed := uint32(1); seed <= 12; seed++ {
		g := doorGame()
		g.RNG = rng.NewWithSeed(seed)
		g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_closed"])
		g.Level.At(Pos{2, 1}).Visible = true
		ghoul := &content.MonsterDef{ID: "ghoul", Name: "ghoul", HP: 3, Damage: "1d1", NoDoors: true, DoorNoise: "moaning behind"}
		g.Level.Creatures = append(g.Level.Creatures, &Creature{Def: ghoul, Pos: Pos{3, 1}, HP: 3})
		g.worldTick()
		if g.Level.At(Pos{2, 1}).Def.ID != "door_closed" {
			t.Fatalf("seed %d: a ghoul opened the door", seed)
		}
		heard = heard || hasMessage(g, "You hear something moaning behind the door.")
	}
	if !heard {
		t.Error("in twelve tries nobody heard the ghoul at the door")
	}
}

// A player shapeshifted into such a form is told so, and loses no turn.
func TestShapeWithoutDoorsCannotOpen(t *testing.T) {
	g := doorGame()
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_closed"])
	g.Shape = &content.MonsterDef{ID: "slime", Name: "slime", NoDoors: true}
	g.PlayerStep(DirE)
	if g.Level.At(Pos{2, 1}).Def.ID != "door_closed" {
		t.Error("a slime-shaped player opened the door")
	}
	if !hasMessage(g, "As a slime, you cannot open doors.") {
		t.Errorf("expected the C refusal, got %v", g.Messages)
	}
}

// A doorless form is refused before the locked-door chain and can't close,
// unlock, or force doors either (C doors.c:54, :219, :412).
func TestShapeWithoutDoorsRefusesEveryDoorVerb(t *testing.T) {
	g := doorGame()
	g.Content.Tiles["door_locked"] = &content.TileDef{ID: "door_locked", Glyph: "+", Locked: true}
	g.Content.Tiles["door_open"].ClosesTo = "door_closed"
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_locked"])
	g.Shape = &content.MonsterDef{ID: "slime", Name: "slime", NoDoors: true}
	g.PlayerStep(DirE)
	if _, ok := g.TakeLockedBump(); ok {
		t.Error("a slime shouldn't reach the unlock/force prompts")
	}
	g.ForceDoor(Pos{2, 1})
	if g.Level.At(Pos{2, 1}).Def.ID != "door_locked" {
		t.Error("a slime forced the door")
	}
	g.Level.Set(Pos{1, 0}, g.Content.Tiles["door_open"])
	g.CloseDoor(Pos{1, 0})
	if g.Level.At(Pos{1, 0}).Def.ID != "door_open" || !hasMessage(g, "As a slime, you cannot close doors.") {
		t.Errorf("a slime closed a door, messages %v", g.Messages)
	}
}
