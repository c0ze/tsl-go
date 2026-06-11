package game

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/rng"
)

func chainContent() *content.Content {
	return &content.Content{
		Tiles: map[string]*content.TileDef{
			"floor":       {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
			"wall":        {ID: "wall", Glyph: "#", Color: content.ColorNormal},
			"door_open":   {ID: "door_open", Glyph: "'", Color: content.ColorBrown, Passable: true, Transparent: true, ClosesTo: "door_closed"},
			"door_closed": {ID: "door_closed", Glyph: "+", Color: content.ColorBrown, OpensTo: "door_open"},
			"door_secret": {ID: "door_secret", Glyph: "#", Color: content.ColorNormal, Secret: true},
			"door_locked": {ID: "door_locked", Glyph: "+", Color: content.ColorBrown, Locked: true},
		},
		Items: map[string]*content.ItemDef{
			"key":     {ID: "key", Name: "key", Glyph: "'", Color: content.ColorNormal, Kind: "tool", Weight: 1},
			"crowbar": {ID: "crowbar", Name: "crowbar", Glyph: ")", Color: content.ColorNormal, Kind: "weapon", Damage: "1d3"},
		},
	}
}

func chainGame(seed uint32) *Game {
	c := chainContent()
	l := NewLevel(7, 5, c.Tiles["floor"])
	return &Game{Content: c, Level: l, Player: Pos{1, 1}, RNG: rng.NewWithSeed(seed), PlayerHP: 10, PlayerMax: 10}
}

func hasMsg(g *Game, want string) bool {
	for _, m := range g.Messages {
		if strings.Contains(m, want) {
			return true
		}
	}
	return false
}

// Bumping a secret door reveals it with the C's line (player.c:1620) and
// maybe_locked_door makes it locked or closed at 50/50 (doors.c).
func TestSecretDoorReveal(t *testing.T) {
	gotLocked, gotClosed := false, false
	for seed := uint32(1); seed <= 20; seed++ {
		g := chainGame(seed)
		g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_secret"])

		g.PlayerStep(DirE)

		if !hasMsg(g, "You find a secret door!") {
			t.Fatalf("seed %d: missing discovery message, got %v", seed, g.Messages)
		}
		if g.Player != (Pos{1, 1}) {
			t.Fatalf("seed %d: player moved into the secret door, at %v", seed, g.Player)
		}
		switch g.Level.At(Pos{2, 1}).Def.ID {
		case "door_locked":
			gotLocked = true
		case "door_closed":
			gotClosed = true
		default:
			t.Fatalf("seed %d: secret door became %q", seed, g.Level.At(Pos{2, 1}).Def.ID)
		}
	}
	if !gotLocked || !gotClosed {
		t.Errorf("maybe_locked_door should land both ways over 20 seeds: locked=%v closed=%v", gotLocked, gotClosed)
	}
}

// Bumping a locked door says so (doors.c:278) and parks the position for the
// front-end's prompt chain; it never opens via the plain bump path.
func TestLockedDoorBump(t *testing.T) {
	g := chainGame(1)
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_locked"])

	g.PlayerStep(DirE)

	if !hasMsg(g, "It is locked.") {
		t.Fatalf("missing locked message, got %v", g.Messages)
	}
	if g.Level.At(Pos{2, 1}).Def.ID != "door_locked" {
		t.Errorf("bump must not open a locked door, got %q", g.Level.At(Pos{2, 1}).Def.ID)
	}
	pos, ok := g.TakeLockedBump()
	if !ok || pos != (Pos{2, 1}) {
		t.Errorf("TakeLockedBump = %v, %v; want {2 1}, true", pos, ok)
	}
	if _, again := g.TakeLockedBump(); again {
		t.Error("TakeLockedBump should clear after one take")
	}
}

// unlock_door (doors.c) destroys exactly one key and opens the door outright.
func TestUnlockDoorConsumesOneKey(t *testing.T) {
	g := chainGame(1)
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_locked"])
	key := g.Content.Items["key"]
	g.Inventory = append(g.Inventory, &Item{Def: key}, &Item{Def: key})

	g.UnlockDoor(Pos{2, 1})

	if !hasMsg(g, "You unlock the door.") {
		t.Fatalf("missing unlock message, got %v", g.Messages)
	}
	if g.Level.At(Pos{2, 1}).Def.ID != "door_open" {
		t.Errorf("unlocking should open the door (C sets tile_door_open), got %q", g.Level.At(Pos{2, 1}).Def.ID)
	}
	if n := g.KeyCount(); n != 1 {
		t.Errorf("unlocking should consume exactly one key, %d left", n)
	}
}

// A crowbar breaks a locked door open every time (doors.c:307), leaving floor.
func TestForceDoorWithCrowbar(t *testing.T) {
	g := chainGame(1)
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_locked"])
	g.Inventory = append(g.Inventory, &Item{Def: g.Content.Items["crowbar"]})

	g.ForceDoor(Pos{2, 1})

	if !hasMsg(g, "You break the door open with your crowbar!") {
		t.Fatalf("missing crowbar message, got %v", g.Messages)
	}
	if g.Level.At(Pos{2, 1}).Def.ID != "floor" {
		t.Errorf("a broken door becomes floor, got %q", g.Level.At(Pos{2, 1}).Def.ID)
	}
}

// Bare hands force at roll_xn(5,3) = 5 in 8; failure makes loud noise that
// wakes nearby sleepers (doors.c:323 draw_attention).
func TestForceDoorBareHands(t *testing.T) {
	broke, failed := false, false
	for seed := uint32(1); seed <= 40 && !(broke && failed); seed++ {
		g := chainGame(seed)
		g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_locked"])
		rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3, Damage: "1d1"}, Pos: Pos{5, 3}, HP: 3}
		rat.Effects = append(rat.Effects, Effect{Kind: "sleep", Turns: 99})
		g.Level.Creatures = append(g.Level.Creatures, rat)

		g.ForceDoor(Pos{2, 1})

		switch g.Level.At(Pos{2, 1}).Def.ID {
		case "floor":
			if !hasMsg(g, "You break the door open!") {
				t.Fatalf("seed %d: success without its message: %v", seed, g.Messages)
			}
			broke = true
		case "door_locked":
			if !hasMsg(g, "You fail to force the door.") {
				t.Fatalf("seed %d: failure without its message: %v", seed, g.Messages)
			}
			if rat.HasEffect("sleep") {
				t.Errorf("seed %d: the noise of a failed force should wake the rat", seed)
			}
			failed = true
		default:
			t.Fatalf("seed %d: door became %q", seed, g.Level.At(Pos{2, 1}).Def.ID)
		}
	}
	if !broke || !failed {
		t.Errorf("forcing should land both ways over 40 seeds: broke=%v failed=%v", broke, failed)
	}
}

// close_door (doors.c): an open door closes unless something is in the way,
// and the DOOR_STEALTH roll decides a silent close vs a loud slam.
func TestCloseDoor(t *testing.T) {
	silent, slammed := false, false
	for seed := uint32(1); seed <= 30 && !(silent && slammed); seed++ {
		g := chainGame(seed)
		g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_open"])
		rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3, Damage: "1d1"}, Pos: Pos{5, 3}, HP: 3}
		rat.Effects = append(rat.Effects, Effect{Kind: "sleep", Turns: 99})
		g.Level.Creatures = append(g.Level.Creatures, rat)

		g.CloseDoor(Pos{2, 1})

		if g.Level.At(Pos{2, 1}).Def.ID != "door_closed" {
			t.Fatalf("seed %d: closing an open door should yield door_closed, got %q", seed, g.Level.At(Pos{2, 1}).Def.ID)
		}
		switch {
		case hasMsg(g, "You silently close the door."):
			silent = true
		case hasMsg(g, "You slam the door shut."):
			if rat.HasEffect("sleep") {
				t.Errorf("seed %d: a slam is loud and should wake the rat", seed)
			}
			slammed = true
		default:
			t.Fatalf("seed %d: a successful close must report itself, got %v", seed, g.Messages)
		}
	}
	if !silent || !slammed {
		t.Errorf("the DOOR_STEALTH roll should land both ways over 30 seeds: silent=%v slammed=%v", silent, slammed)
	}
}

func TestCloseDoorBlocked(t *testing.T) {
	g := chainGame(1)
	g.Level.Set(Pos{2, 1}, g.Content.Tiles["door_open"])
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3, Damage: "1d1"}, Pos: Pos{2, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)

	g.CloseDoor(Pos{2, 1})

	if !hasMsg(g, "There is something in the way.") {
		t.Fatalf("missing blocked message, got %v", g.Messages)
	}
	if g.Level.At(Pos{2, 1}).Def.ID != "door_open" {
		t.Error("a blocked door must stay open")
	}

	g2 := chainGame(1)
	g2.Level.Set(Pos{2, 1}, g2.Content.Tiles["door_open"])
	g2.Level.Items = append(g2.Level.Items, &Item{Def: g2.Content.Items["key"], Pos: Pos{2, 1}})
	g2.CloseDoor(Pos{2, 1})
	if g2.Level.At(Pos{2, 1}).Def.ID != "door_open" {
		t.Error("an item in the doorway must block closing")
	}
}

// Monsters never open locked or secret doors (doors.c bails for non-players).
func TestMonsterBlockedByLockedDoor(t *testing.T) {
	for _, tile := range []string{"door_locked", "door_secret"} {
		g := chainGame(1)
		g.Level.Set(Pos{2, 1}, g.Content.Tiles[tile])
		rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3, Damage: "1d1"}, Pos: Pos{3, 1}, HP: 3}
		g.Level.Creatures = append(g.Level.Creatures, rat)

		g.worldTick()

		if got := g.Level.At(Pos{2, 1}).Def.ID; got != tile {
			t.Errorf("a monster must not open %s, got %q", tile, got)
		}
	}
}
