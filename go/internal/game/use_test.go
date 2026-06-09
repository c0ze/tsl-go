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

func TestPickupAutoEquipsWhenSlotEmpty(t *testing.T) {
	g := useGame()
	sword := &Item{Def: &content.ItemDef{ID: "sword", Name: "sword", Kind: "weapon", Attack: 4, Damage: "2d4"}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, sword)
	g.PlayerPickup()
	if g.Weapon != sword {
		t.Fatal("first weapon should auto-equip when the slot is empty")
	}
	// a second weapon must NOT auto-replace the equipped one
	dagger := &Item{Def: &content.ItemDef{ID: "dagger", Name: "dagger", Kind: "weapon", Attack: 2, Damage: "1d4"}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, dagger)
	g.PlayerPickup()
	if g.Weapon != sword {
		t.Error("second weapon should not auto-replace the equipped one")
	}
	if len(g.Inventory) != 2 {
		t.Errorf("both weapons should be carried, inventory = %d", len(g.Inventory))
	}
	// armor auto-equips into the empty armor slot
	mail := &Item{Def: &content.ItemDef{ID: "chainmail", Name: "chainmail", Kind: "armor", Dodge: 4}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, mail)
	g.PlayerPickup()
	if g.Armor != mail {
		t.Error("armor should auto-equip when the slot is empty")
	}
}

func TestUseRingEquipsIntoSlot(t *testing.T) {
	g := useGame()
	ring := &Item{Def: &content.ItemDef{ID: "ring_protect", Name: "ring of protection", Kind: "ring", Dodge: 2}}
	g.Inventory = append(g.Inventory, ring)
	g.PlayerUse(ring)
	if g.Ring != ring {
		t.Fatal("ring should be worn in the ring slot")
	}
	if got := g.playerDodgeStat(); got != playerDodge+2 {
		t.Errorf("dodge = %d, want %d (base + ring)", got, playerDodge+2)
	}
}

func TestUseAmuletEquipsIntoSlot(t *testing.T) {
	g := useGame()
	amulet := &Item{Def: &content.ItemDef{ID: "amulet_ward", Name: "amulet of warding", Kind: "amulet", Dodge: 3}}
	g.Inventory = append(g.Inventory, amulet)
	g.PlayerUse(amulet)
	if g.Amulet != amulet {
		t.Fatal("amulet should be worn in the amulet slot")
	}
	if got := g.playerDodgeStat(); got != playerDodge+3 {
		t.Errorf("dodge = %d, want %d (base + amulet)", got, playerDodge+3)
	}
}

// Accessory bonuses stack with armor (dodge) and weapon (attack) — the point of
// separate equip slots.
func TestAccessoryBonusesStack(t *testing.T) {
	g := useGame()
	g.Armor = &Item{Def: &content.ItemDef{Kind: "armor", Dodge: 4}}
	g.Ring = &Item{Def: &content.ItemDef{Kind: "ring", Dodge: 2}}
	g.Amulet = &Item{Def: &content.ItemDef{Kind: "amulet", Dodge: 3}}
	if got, want := g.playerDodgeStat(), playerDodge+4+2+3; got != want {
		t.Errorf("dodge = %d, want %d (base + armor + ring + amulet)", got, want)
	}
	g.Weapon = &Item{Def: &content.ItemDef{Kind: "weapon", Attack: 4, Damage: "1d4"}}
	g.Ring = &Item{Def: &content.ItemDef{Kind: "ring", Attack: 2}}
	if got, want := g.playerAttackStat(), playerAttack+4+2; got != want {
		t.Errorf("attack = %d, want %d (base + weapon + ring)", got, want)
	}
}

func TestPickupAutoEquipsAccessories(t *testing.T) {
	g := useGame()
	ring := &Item{Def: &content.ItemDef{ID: "r", Name: "ring", Kind: "ring", Dodge: 2}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, ring)
	g.PlayerPickup()
	if g.Ring != ring {
		t.Fatal("ring should auto-equip into the empty ring slot")
	}
	amulet := &Item{Def: &content.ItemDef{ID: "a", Name: "amulet", Kind: "amulet", Dodge: 3}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, amulet)
	g.PlayerPickup()
	if g.Amulet != amulet {
		t.Fatal("amulet should auto-equip into the empty amulet slot")
	}
	// a second ring must NOT auto-replace the worn one
	ring2 := &Item{Def: &content.ItemDef{ID: "r2", Name: "ring of accuracy", Kind: "ring", Attack: 2}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, ring2)
	g.PlayerPickup()
	if g.Ring != ring {
		t.Error("second ring should not auto-replace the worn one")
	}
}

// Re-using a ring while one is worn swaps the slot (a deliberate change).
func TestUseRingSwapsSlot(t *testing.T) {
	g := useGame()
	r1 := &Item{Def: &content.ItemDef{ID: "r1", Name: "ring of protection", Kind: "ring", Dodge: 2}}
	r2 := &Item{Def: &content.ItemDef{ID: "r2", Name: "ring of accuracy", Kind: "ring", Attack: 2}}
	g.Inventory = append(g.Inventory, r1, r2)
	g.PlayerUse(r1)
	g.PlayerUse(r2)
	if g.Ring != r2 {
		t.Errorf("ring slot should hold the most recently worn ring")
	}
}

func TestWandInventoryFilters(t *testing.T) {
	g := useGame()
	g.Inventory = append(g.Inventory,
		&Item{Def: &content.ItemDef{Name: "dagger", Kind: "weapon"}},
		&Item{Def: &content.ItemDef{Name: "wand", Kind: "wand", Damage: "1d1"}, Charges: 3},
	)
	wands := g.WandInventory()
	if len(wands) != 1 || wands[0].Def.Name != "wand" {
		t.Errorf("WandInventory = %v, want [wand]", wands)
	}
}
