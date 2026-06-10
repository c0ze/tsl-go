package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestFireWeaponKillsInRange(t *testing.T) {
	g := rangedGame() // player at (1,1), all floor
	quiver(g, 5)
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{5, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)

	g.FireWeapon(Pos{5, 1})

	if g.Level.CreatureAt(Pos{5, 1}) != nil {
		t.Error("a ranged weapon should kill a creature in range with line of sight")
	}
}

func TestFireWeaponOutOfRange(t *testing.T) {
	g := combatGame()
	g.Weapon = &Item{Def: &content.ItemDef{Name: "shortbow", Kind: "weapon", Damage: "10d1", Ranged: 2}}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)

	g.FireWeapon(Pos{8, 1})

	if g.Level.CreatureAt(Pos{8, 1}) == nil {
		t.Error("an out-of-range shot should not hit")
	}
}

func TestFireWeaponBlockedByWall(t *testing.T) {
	g := combatGame()
	g.Weapon = &Item{Def: &content.ItemDef{Name: "shortbow", Kind: "weapon", Damage: "10d1", Ranged: 8}}
	g.Level.Set(Pos{3, 1}, g.Content.Tiles["wall"])
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{5, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)

	g.FireWeapon(Pos{5, 1})

	if g.Level.CreatureAt(Pos{5, 1}) == nil {
		t.Error("a shot with no clear line of sight should not hit")
	}
}

func TestFireWeaponNoRangedWeapon(t *testing.T) {
	g := combatGame()
	g.Weapon = &Item{Def: &content.ItemDef{Name: "dagger", Kind: "weapon", Damage: "1d4"}} // Ranged 0
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{5, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)

	g.FireWeapon(Pos{5, 1})

	if g.Level.CreatureAt(Pos{5, 1}) == nil {
		t.Error("with no ranged weapon wielded, firing should hit nothing")
	}
	if !g.WieldedRanged() {
		// sanity: a plain dagger is not a ranged weapon
	} else {
		t.Error("a melee dagger should not count as a ranged weapon")
	}
}

// rangedGame is a combat corridor with a wielded shortbow (arrows added per
// test via quiver).
func rangedGame() *Game {
	g := combatGame()
	g.Weapon = &Item{Def: &content.ItemDef{Name: "shortbow", Kind: "weapon", Damage: "10d1", Ranged: 6}}
	return g
}

func quiver(g *Game, count int) *Item {
	arrows := &Item{Def: &content.ItemDef{ID: "arrow", Name: "crude arrow", Kind: "ammo"}, Charges: count}
	g.Inventory = append(g.Inventory, arrows)
	return arrows
}

func TestFiringConsumesAnArrow(t *testing.T) {
	g := rangedGame()
	arrows := quiver(g, 3)
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{4, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.FireWeapon(rat.Pos)
	if arrows.Charges != 2 {
		t.Errorf("a shot spends one arrow (C throw_or_fire), %d left", arrows.Charges)
	}
	if rat.HP >= 9 {
		t.Error("the shot should still land")
	}
}

func TestEmptyQuiverRefusesFree(t *testing.T) {
	g := rangedGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{4, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.FireWeapon(rat.Pos) // no arrows at all
	if rat.HP != 9 {
		t.Error("no arrow, no shot")
	}
	if !hasMessage(g, "Out of ammo!") {
		t.Errorf("expected the C refusal, got %v", g.Messages)
	}
}

func TestLastArrowEmptiesTheQuiver(t *testing.T) {
	g := rangedGame()
	quiver(g, 1)
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 99, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{4, 1}, HP: 99}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.FireWeapon(rat.Pos)
	for _, it := range g.Inventory {
		if it.Def.Kind == "ammo" {
			t.Fatal("an emptied stack should leave the pack")
		}
	}
}

func TestPickupMergesArrowStacks(t *testing.T) {
	g := rangedGame()
	def := &content.ItemDef{ID: "arrow", Name: "crude arrow", Kind: "ammo", Power: 8}
	g.Inventory = append(g.Inventory, &Item{Def: def, Charges: 5})
	g.Level.Items = append(g.Level.Items, &Item{Def: def, Charges: 8, Pos: g.Player})
	g.PlayerPickup()
	stacks, total := 0, 0
	for _, it := range g.Inventory {
		if it.Def.Kind == "ammo" {
			stacks++
			total += it.Charges
		}
	}
	if stacks != 1 || total != 13 {
		t.Errorf("bundles merge into one quiver: %d stacks holding %d", stacks, total)
	}
}
