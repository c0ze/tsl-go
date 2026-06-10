package game

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

// loadedGame is a combat corridor with a pursuing rat and a heavy lump in the
// player's pack (one C corpse outweighs the whole 400 allowance by itself).
func loadedGame() (*Game, *Creature) {
	g, rat := schedulerGame()
	g.Inventory = append(g.Inventory, &Item{Def: &content.ItemDef{ID: "corpse", Name: "corpse", Kind: "food", Weight: 599}})
	return g, rat
}

func TestBurdenedPlayerGivesMonstersTwoTicks(t *testing.T) {
	g, rat := loadedGame()
	g.advanceWorld()
	if rat.Pos.X != 6 {
		t.Errorf("a burdened player moves at half speed (C BURDENED_FACTOR): rat should be at x=6, got x=%d", rat.Pos.X)
	}
}

func TestStaggerOnPickupAndReliefOnUse(t *testing.T) {
	g := combatGame()
	corpse := &Item{Def: &content.ItemDef{ID: "corpse", Name: "corpse", Kind: "food", Weight: 599, Use: "eat"}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, corpse)
	g.PlayerPickup()
	if !hasMessage(g, "You stagger under your load!") {
		t.Fatalf("crossing the allowance staggers (C burdened.c), got %v", g.Messages)
	}
	if got := g.EffectsSummary(); !strings.Contains(got, "Burdened") {
		t.Errorf("HUD should flag the burden, got %q", got)
	}
	g.Behaviors = map[string]Behavior{"eat": func(g *Game, it *Item) []string { return nil }}
	g.PlayerUse(corpse) // consuming it drops the weight
	if !hasMessage(g, "You are no longer burdened.") {
		t.Errorf("dropping under the allowance announces relief, got %v", g.Messages)
	}
	if strings.Contains(g.EffectsSummary(), "Burdened") {
		t.Error("HUD flag should clear with the load")
	}
}

func TestLightPackNoBurden(t *testing.T) {
	g, rat := schedulerGame()
	g.Inventory = append(g.Inventory, &Item{Def: &content.ItemDef{ID: "healing_potion", Kind: "potion", Weight: 7}})
	g.advanceWorld()
	if rat.Pos.X != 7 {
		t.Errorf("a light pack costs nothing: rat should be at x=7, got x=%d", rat.Pos.X)
	}
}

func TestAmmoStackWeighsPerArrow(t *testing.T) {
	g, rat := schedulerGame()
	// 500 arrows at WEIGHT_MISSILE 1 each: 500 > the 400 allowance
	// (C burdened.c: carried = stack_size * weight).
	g.Inventory = append(g.Inventory, &Item{Def: &content.ItemDef{ID: "arrow", Name: "crude arrow", Kind: "ammo", Weight: 1}, Charges: 500})
	g.advanceWorld()
	if rat.Pos.X != 6 {
		t.Errorf("an overloaded quiver burdens like anything else: rat should be at x=6, got x=%d", rat.Pos.X)
	}
}

func TestMergedPickupCanStagger(t *testing.T) {
	g := combatGame()
	def := &content.ItemDef{ID: "arrow", Name: "crude arrow", Kind: "ammo", Weight: 1, Power: 8}
	g.Inventory = append(g.Inventory, &Item{Def: def, Charges: 390})
	g.Level.Items = append(g.Level.Items, &Item{Def: def, Charges: 50, Pos: g.Player})
	g.PlayerPickup() // merge crosses the 400 line
	if !hasMessage(g, "You stagger under your load!") {
		t.Errorf("a merged bundle that crosses the allowance staggers too, got %v", g.Messages)
	}
}
