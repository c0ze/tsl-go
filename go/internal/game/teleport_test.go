package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

func TestTeleportMovesToPassableTile(t *testing.T) {
	g := combatGame() // 10x3 all floor, player at (1,1)
	start := g.Player
	if !g.Teleport() {
		t.Fatal("teleport should succeed on an open level")
	}
	if g.Player == start {
		t.Error("teleport should move the player off the starting tile")
	}
	if !g.Level.Passable(g.Player) {
		t.Errorf("teleport landed on an impassable tile %v", g.Player)
	}
	if g.Level.CreatureAt(g.Player) != nil {
		t.Error("teleport should not land on a creature")
	}
}

func TestTeleportLandsOnFloorAmongWalls(t *testing.T) {
	c := testContent()
	rows := []string{
		"#####",
		"#@.##",
		"##..#",
		"#####",
	}
	lvl, start, err := ParseLevel(c, rows, map[rune]string{'#': "wall", '@': "floor", '.': "floor"})
	if err != nil {
		t.Fatal(err)
	}
	g := &Game{Content: c, Level: lvl, Player: start, RNG: rng.NewWithSeed(1)}
	for i := 0; i < 30; i++ {
		g.Teleport()
		if !g.Level.Passable(g.Player) {
			t.Fatalf("teleport landed on a wall at %v", g.Player)
		}
	}
}

func TestRevealMapMarksAllSeen(t *testing.T) {
	g := combatGame()
	g.RevealMap()
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			if !g.Level.At(Pos{X: x, Y: y}).Seen {
				t.Fatalf("tile %d,%d not marked seen after RevealMap", x, y)
			}
		}
	}
}

func TestReadableInventoryFiltersScrolls(t *testing.T) {
	g := combatGame()
	g.Inventory = []*Item{
		{Def: &content.ItemDef{Name: "potion", Kind: "potion", Use: "heal"}},
		{Def: &content.ItemDef{Name: "scroll", Kind: "scroll", Use: "teleport"}},
	}
	r := g.ReadableInventory()
	if len(r) != 1 || r[0].Def.Kind != "scroll" {
		t.Errorf("ReadableInventory = %v, want exactly one scroll", r)
	}
}

func TestPlayerUseScrollConsumesAndRuns(t *testing.T) {
	g := combatGame()
	ran := false
	g.Behaviors = map[string]Behavior{"teleport": func(gg *Game, it *Item) []string {
		ran = true
		return []string{"zap"}
	}}
	sc := &Item{Def: &content.ItemDef{Name: "scroll", Kind: "scroll", Use: "teleport"}}
	g.Inventory = append(g.Inventory, sc)

	g.PlayerUse(sc)

	if !ran {
		t.Error("reading a scroll should invoke its behavior")
	}
	if g.hasInventoryItem(sc) {
		t.Error("a read scroll should be consumed")
	}
}
