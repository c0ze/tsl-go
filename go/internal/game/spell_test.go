package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestCastSpellSpendsEP(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 10, 10
	cast := false
	g.Behaviors = map[string]Behavior{"first_aid": func(gg *Game, it *Item) []string { cast = true; return []string{"mend"} }}
	book := &Item{Def: &content.ItemDef{Name: "spellbook of first aid", Kind: "spellbook", Use: "first_aid", Cost: 4}}
	g.Inventory = append(g.Inventory, book)

	g.CastSpell(book)

	if g.EP != 6 {
		t.Errorf("EP = %d, want 6 after casting a cost-4 spell", g.EP)
	}
	if !cast {
		t.Error("casting should invoke the spell behavior")
	}
}

func TestCastSpellRefusedWhenLowEP(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 2, 10
	cast := false
	g.Behaviors = map[string]Behavior{"first_aid": func(gg *Game, it *Item) []string { cast = true; return nil }}
	book := &Item{Def: &content.ItemDef{Name: "book", Kind: "spellbook", Use: "first_aid", Cost: 4}}
	g.Inventory = append(g.Inventory, book)

	g.CastSpell(book)

	if g.EP != 2 {
		t.Errorf("EP should be unchanged at 2 when you can't afford the spell, got %d", g.EP)
	}
	if cast {
		t.Error("a spell you can't afford should not be cast")
	}
}

func TestEPRegensOverTurnsAndClamps(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 0, 5
	for i := 0; i < 9; i++ {
		g.regenEP()
	}
	if g.EP < 1 || g.EP > 5 {
		t.Errorf("EP should regenerate toward max over turns, got %d", g.EP)
	}
	g.EP = g.EPMax
	for i := 0; i < 3; i++ {
		g.regenEP()
	}
	if g.EP != g.EPMax {
		t.Errorf("EP should clamp at max, got %d", g.EP)
	}
}

func TestCastSpellAtDamages(t *testing.T) {
	g := combatGame() // player at (1,1), all floor
	g.EP, g.EPMax = 10, 10
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{4, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	book := &Item{Def: &content.ItemDef{Name: "spellbook of force bolt", Kind: "spellbook", Ranged: 6, Damage: "10d1", Cost: 5}}

	g.CastSpellAt(book, Pos{4, 1})

	if g.Level.CreatureAt(Pos{4, 1}) != nil {
		t.Error("force bolt should kill the rat (10 dmg vs 3 HP)")
	}
	if g.EP != 5 {
		t.Errorf("EP = %d, want 5 after a cost-5 cast", g.EP)
	}
}

func TestCastSpellAtRefusedConditions(t *testing.T) {
	mk := func() (*Game, *Item, Pos) {
		g := combatGame()
		g.EP, g.EPMax = 10, 10
		rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{4, 1}, HP: 3}
		g.Level.Creatures = append(g.Level.Creatures, rat)
		book := &Item{Def: &content.ItemDef{Name: "bolt", Kind: "spellbook", Ranged: 6, Damage: "10d1", Cost: 5}}
		return g, book, rat.Pos
	}

	g, book, pos := mk()
	g.EP = 2
	g.CastSpellAt(book, pos)
	if g.EP != 2 || g.Level.CreatureAt(pos) == nil {
		t.Error("a spell you can't afford should not fire or spend EP")
	}

	g, book, pos = mk()
	book.Def.Ranged = 1 // out of range (distance 3)
	g.CastSpellAt(book, pos)
	if g.EP != 10 || g.Level.CreatureAt(pos) == nil {
		t.Error("an out-of-range cast should be refused (no EP spent)")
	}

	g, book, pos = mk()
	g.Level.Set(Pos{3, 1}, g.Content.Tiles["wall"]) // block the line
	g.CastSpellAt(book, pos)
	if g.EP != 10 || g.Level.CreatureAt(pos) == nil {
		t.Error("a cast with no line of sight should be refused (no EP spent)")
	}
}

func TestSpellInventoryFiltersSpellbooks(t *testing.T) {
	g := combatGame()
	g.Inventory = []*Item{
		{Def: &content.ItemDef{Name: "potion", Kind: "potion", Use: "heal"}},
		{Def: &content.ItemDef{Name: "book", Kind: "spellbook", Use: "first_aid", Cost: 4}},
	}
	s := g.SpellInventory()
	if len(s) != 1 || s[0].Def.Kind != "spellbook" {
		t.Errorf("SpellInventory should list only spellbooks, got %v", s)
	}
}
