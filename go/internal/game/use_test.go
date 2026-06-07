package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

func useGame() *Game {
	c := testContent()
	return &Game{
		Content: c, Level: NewLevel(8, 3, c.Tiles["floor"]), Player: Pos{1, 1},
		RNG: rng.NewWithSeed(1), PlayerHP: 10, PlayerMax: 20,
		Behaviors: map[string]Behavior{
			"heal": func(g *Game, it *Item) []string {
				g.PlayerHP += it.Def.Power
				if g.PlayerHP > g.PlayerMax {
					g.PlayerHP = g.PlayerMax
				}
				return []string{"healed"}
			},
		},
	}
}

func TestPickupMovesItemToInventory(t *testing.T) {
	g := useGame()
	it := &Item{Def: &content.ItemDef{ID: "dagger", Name: "dagger", Glyph: ")", Kind: "weapon"}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, it)
	g.PlayerPickup()
	if len(g.Inventory) != 1 || g.Inventory[0] != it {
		t.Fatal("item should be in inventory")
	}
	if g.Level.ItemAt(g.Player) != nil {
		t.Error("item should be off the ground")
	}
}

func TestUseWeaponEquipsAndBoostsDamage(t *testing.T) {
	g := useGame()
	dagger := &Item{Def: &content.ItemDef{ID: "dagger", Name: "dagger", Kind: "weapon", Attack: 3, Damage: "5d1"}}
	g.Inventory = append(g.Inventory, dagger)
	g.PlayerUse(dagger)
	if g.Weapon != dagger {
		t.Fatal("dagger should be equipped")
	}
	if g.playerDamageSpec() != "5d1" {
		t.Errorf("damage spec = %q, want 5d1 from weapon", g.playerDamageSpec())
	}
}

func TestUsePotionHealsAndIsConsumed(t *testing.T) {
	g := useGame()
	potion := &Item{Def: &content.ItemDef{ID: "healing_potion", Name: "healing potion", Kind: "potion", Use: "heal", Power: 5}}
	g.Inventory = append(g.Inventory, potion)
	g.PlayerUse(potion)
	if g.PlayerHP != 15 {
		t.Errorf("HP = %d, want 15 (10+5)", g.PlayerHP)
	}
	if len(g.Inventory) != 0 {
		t.Error("potion should be consumed")
	}
}

func TestPlayerUseFoodEatsAndRemoves(t *testing.T) {
	g := useGame()
	g.Behaviors["eat"] = func(gg *Game, it *Item) []string {
		gg.PlayerHP += it.Def.Power
		return []string{"munch"}
	}
	food := &Item{Def: &content.ItemDef{Name: "rat corpse", Kind: "food", Use: "eat", Power: 3}}
	g.Inventory = append(g.Inventory, food)
	g.PlayerUse(food)
	if g.PlayerHP != 13 {
		t.Errorf("HP = %d, want 13 (10+3)", g.PlayerHP)
	}
	if len(g.Inventory) != 0 {
		t.Errorf("food should be consumed, inventory = %v", g.Inventory)
	}
}

func TestValidateItemUsesCoversFood(t *testing.T) {
	c := &content.Content{Items: map[string]*content.ItemDef{
		"corpse": {ID: "corpse", Kind: "food", Use: "no_such_behavior"},
	}}
	if err := ValidateItemUses(c, map[string]Behavior{}); err == nil {
		t.Fatal("expected error: food references unknown behavior")
	}
}

func TestEdibleInventoryFiltersFood(t *testing.T) {
	g := useGame()
	g.Inventory = append(g.Inventory,
		&Item{Def: &content.ItemDef{Name: "dagger", Kind: "weapon"}},
		&Item{Def: &content.ItemDef{Name: "ration", Kind: "food", Use: "eat"}},
	)
	food := g.EdibleInventory()
	if len(food) != 1 || food[0].Def.Name != "ration" {
		t.Errorf("EdibleInventory = %v, want [ration]", food)
	}
}
