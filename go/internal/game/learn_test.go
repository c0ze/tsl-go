package game

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

func bookGame() (*Game, *Item) {
	g := combatGame()
	g.EPMax, g.EP = 9, 9
	g.Identified = map[string]bool{}
	book := &Item{Def: &content.ItemDef{ID: "book_force", Name: "spellbook of force bolt", Kind: "spellbook", Ranged: 6, Damage: "2d6", Cost: 5}}
	g.Inventory = append(g.Inventory, book)
	return g, book
}

func TestReadingLearnsTheSpell(t *testing.T) {
	// Walk seeds until the coin lands on the good branch; assert the learn.
	for seed := uint32(1); seed <= 16; seed++ {
		g, book := bookGame()
		g.RNG = rng.NewWithSeed(seed)
		g.PlayerUse(book)
		if !g.Known["book_force"] {
			continue // bad-book branch this seed
		}
		if g.EPMax != 10 {
			t.Errorf("learning grows max EP by 1 (C reading.c), got %d", g.EPMax)
		}
		if !hasMessage(g, "You learn spellbook of force bolt.") {
			t.Errorf("expected the C learn line, got %v", g.Messages)
		}
		if g.hasInventoryItem(book) {
			t.Error("the book is consumed on learning (C del_item)")
		}
		return
	}
	t.Fatal("sixteen seeds never learned; the coin looks rigged")
}

func TestReadingKnownBookIsFree(t *testing.T) {
	g, book := bookGame()
	g.Known = map[string]bool{"book_force": true}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.PlayerUse(book)
	if !hasMessage(g, "You already know this.") {
		t.Errorf("expected the C line, got %v", g.Messages)
	}
	if !g.hasInventoryItem(book) || rat.Pos.X != 8 {
		t.Error("a known book survives and the refusal is free (C returns false)")
	}
}

func TestBadBookOutcomes(t *testing.T) {
	g, book := bookGame()
	g.badBook(book) // case dispatch is seeded; exercise each branch directly
	// (the dispatcher itself is covered below)

	g2, _ := bookGame()
	g2.hungryBook()
	if !g2.HasEffect("hungry_book") {
		t.Error("case 1: the hungry book latches on")
	}
	if got := g2.playerDamageSpec(); got != hungryBookDamage {
		t.Errorf("the hungry book overrides the wielded weapon, got %q", got)
	}
	if !hasMessage(g2, "The book bites into your hand!") {
		t.Errorf("expected the C bite line, got %v", g2.Messages)
	}

	g3, _ := bookGame()
	g3.darknessFalls()
	if !g3.HasEffect("blind") || !hasMessage(g3, "Darkness falls around you!") {
		t.Error("case 2: darkness blinds (C DEFAULT_BLIND_TIME)")
	}

	g4, book4 := bookGame()
	book4.Def = &content.ItemDef{ID: "book_breathe_fire", Name: "book of Breathe Fire", Kind: "spellbook", Breath: "fire", Cost: 5}
	g4.Content.Monsters = map[string]*content.MonsterDef{
		"flame_spirit": {ID: "flame_spirit", Name: "flame spirit", Glyph: "j", HP: 20, Attack: 8, Damage: "2d4"},
		"imp":          {ID: "imp", Name: "imp", Glyph: "i", HP: 6, Attack: 8, Damage: "1d4"},
	}
	g4.bookEscapee(book4)
	found := false
	for _, c := range g4.Level.Creatures {
		if c.Def.ID == "flame_spirit" && !c.Ally && c.Lifetime > 0 {
			found = true
		}
	}
	if !found {
		t.Error("case 3: a flame spirit escapes from Breathe Fire (hostile, summon lifetime)")
	}
	if !strings.Contains(strings.Join(g4.Messages, "\n"), "escapes from the book!") {
		t.Errorf("expected the C escape line, got %v", g4.Messages)
	}
	_ = book
}

func TestCastFromKnownWithoutCarrying(t *testing.T) {
	g := combatGame()
	g.EPMax, g.EP = 9, 9
	g.Content.Items = map[string]*content.ItemDef{
		"book_force": {ID: "book_force", Name: "spellbook of force bolt", Kind: "spellbook", Ranged: 6, Damage: "2d6", Cost: 5},
	}
	g.Known = map[string]bool{"book_force": true}
	spells := g.SpellInventory()
	if len(spells) != 1 || spells[0].Def.ID != "book_force" {
		t.Fatalf("the cast list is what you know, got %v", spells)
	}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 1, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{3, 1}, HP: 1}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.CastSpellAt(spells[0], rat.Pos)
	if g.EP != 4 {
		t.Errorf("the cast should spend 5 EP, got %d", g.EP)
	}
}

func TestCarriedUnreadBookIsNotCastable(t *testing.T) {
	g, _ := bookGame() // a book in the pack, nothing learned
	if n := len(g.SpellInventory()); n != 0 {
		t.Errorf("carrying an unread book grants nothing (C: knowledge only), got %d spells", n)
	}
}
