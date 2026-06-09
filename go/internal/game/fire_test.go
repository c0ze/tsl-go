package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestFireWeaponKillsInRange(t *testing.T) {
	g := combatGame() // player at (1,1), all floor
	g.Weapon = &Item{Def: &content.ItemDef{Name: "shortbow", Kind: "weapon", Damage: "10d1", Ranged: 6}}
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
