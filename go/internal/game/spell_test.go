package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
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

func TestFrostRayHitsLine(t *testing.T) {
	g := combatGame() // 10x3 all floor, player at (1,1)
	g.EP, g.EPMax = 10, 10
	a := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{3, 1}, HP: 3}
	b := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{5, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, a, b)
	book := &Item{Def: &content.ItemDef{Name: "spellbook of frost ray", Kind: "spellbook", Beam: true, Ranged: 8, Damage: "10d1", Cost: 6}}

	g.CastSpellAt(book, Pos{5, 1}) // aim east, down the line of creatures

	if len(g.Level.Creatures) != 0 {
		t.Errorf("frost ray should strike every creature in the line, %d remain", len(g.Level.Creatures))
	}
	if g.EP != 4 {
		t.Errorf("EP = %d, want 4 after a cost-6 cast", g.EP)
	}
}

func TestFrostRayStoppedByWall(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 10, 10
	g.Level.Set(Pos{4, 1}, g.Content.Tiles["wall"])
	behind := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{5, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, behind)
	book := &Item{Def: &content.ItemDef{Name: "ray", Kind: "spellbook", Beam: true, Ranged: 8, Damage: "10d1", Cost: 6}}

	g.CastSpellAt(book, Pos{5, 1})

	if len(g.Level.Creatures) != 1 {
		t.Error("a wall should stop the beam; the creature behind it should survive")
	}
	if g.EP != 4 {
		t.Errorf("a beam still costs EP even when blocked: EP = %d, want 4", g.EP)
	}
}

func TestFlashBlindBlindsNearby(t *testing.T) {
	g := combatGame()                                                                             // player at (1,1)
	near := &Creature{Def: &content.MonsterDef{ID: "a", Name: "a", HP: 3}, Pos: Pos{3, 1}, HP: 3} // dist 2
	far := &Creature{Def: &content.MonsterDef{ID: "b", Name: "b", HP: 3}, Pos: Pos{9, 1}, HP: 3}  // dist 8
	g.Level.Creatures = append(g.Level.Creatures, near, far)

	n := g.FlashBlind(4, 6)

	if n != 1 {
		t.Errorf("FlashBlind radius 4 should blind 1 creature, got %d", n)
	}
	if !near.HasEffect("blind") {
		t.Error("the near creature should be blinded")
	}
	if far.HasEffect("blind") {
		t.Error("the far creature should be out of range")
	}
}

func TestPoisonNearbyPoisonsAndDamages(t *testing.T) {
	g := combatGame()                                                                             // player at (1,1)
	near := &Creature{Def: &content.MonsterDef{ID: "a", Name: "a", HP: 3}, Pos: Pos{3, 1}, HP: 3} // dist 2
	far := &Creature{Def: &content.MonsterDef{ID: "b", Name: "b", HP: 3}, Pos: Pos{9, 1}, HP: 3}  // dist 8
	g.Level.Creatures = append(g.Level.Creatures, near, far)

	n := g.PoisonNearby(3, 5)

	if n != 1 {
		t.Errorf("PoisonNearby radius 3 should poison 1 creature, got %d", n)
	}
	if !near.HasEffect("poison") {
		t.Error("the near creature should be poisoned")
	}
	if far.HasEffect("poison") {
		t.Error("the far creature should be out of range")
	}
	// poison is damage-over-time: the next turn costs the near creature HP
	hp := near.HP
	g.worldTick()
	if near.HP >= hp {
		t.Errorf("poison should cost the near creature HP: %d -> %d", hp, near.HP)
	}
}

func TestScareNearbyFrightens(t *testing.T) {
	g := combatGame()                                                                             // player at (1,1)
	near := &Creature{Def: &content.MonsterDef{ID: "a", Name: "a", HP: 3}, Pos: Pos{3, 1}, HP: 3} // dist 2
	far := &Creature{Def: &content.MonsterDef{ID: "b", Name: "b", HP: 3}, Pos: Pos{9, 1}, HP: 3}  // dist 8
	g.Level.Creatures = append(g.Level.Creatures, near, far)

	n := g.ScareNearby(3, 5)

	if n != 1 {
		t.Errorf("ScareNearby radius 3 should frighten 1 creature, got %d", n)
	}
	if !near.HasEffect("fear") {
		t.Error("the near creature should be frightened")
	}
	if far.HasEffect("fear") {
		t.Error("the far creature should be out of range")
	}
}

func TestBlindMonsterHoldsAtRange(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 9, Damage: "1d1"}, Pos: Pos{5, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	rat.AddEffect("blind", 10)
	before := rat.Pos

	g.worldTick()

	if rat.Pos != before {
		t.Errorf("a blinded monster should hold position at range, moved to %v", rat.Pos)
	}
}

func TestBlindMonsterStillMeleesAdjacent(t *testing.T) {
	g := combatGame()
	g.PlayerHP = 20
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 9, Attack: 100, Dodge: 0, Damage: "1d2"}, Pos: Pos{2, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	rat.AddEffect("blind", 10)

	g.worldTick()

	if g.PlayerHP >= 20 {
		t.Error("a blinded monster should still attack an adjacent player")
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

func TestDeathspellTheirs(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 5, 5
	book := &Item{Def: &content.ItemDef{Name: "book of Deathspell", Kind: "spellbook", Cost: 1, Deathspell: true}}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 99, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{2, 1}, HP: 99}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	// Seed 1's first Intn(2) lands "Theirs!": the rat dies outright at any HP.
	g.CastSpellAt(book, rat.Pos)
	if g.Dead {
		t.Skip("seed flipped against the caster; covered by TestDeathspellYours")
	}
	if g.Level.CreatureAt(Pos{2, 1}) != nil {
		t.Error("Theirs! — the target dies outright regardless of HP (C deathspell)")
	}
	if g.EP != 4 {
		t.Errorf("the spell costs 1 EP (C attrs.c:355), EP %d", g.EP)
	}
}

func TestDeathspellYours(t *testing.T) {
	// Walk seeds until the flip lands on the caster; both outcomes must occur.
	for seed := uint32(1); seed <= 16; seed++ {
		g := combatGame()
		g.RNG = rng.NewWithSeed(seed)
		g.EP, g.EPMax = 5, 5
		book := &Item{Def: &content.ItemDef{Name: "book of Deathspell", Kind: "spellbook", Cost: 1, Deathspell: true}}
		rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 99, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{2, 1}, HP: 99}
		g.Level.Creatures = append(g.Level.Creatures, rat)
		g.CastSpellAt(book, rat.Pos)
		if g.Dead {
			if g.DeathCause != "deathspell" {
				t.Errorf("morgue cause should be %q, got %q", "deathspell", g.DeathCause)
			}
			if !hasMessage(g, "Yours!") {
				t.Errorf("expected the C line, got %v", g.Messages)
			}
			return
		}
	}
	t.Fatal("sixteen seeds never killed the caster; the coin looks rigged")
}

func TestDeathspellNeedsAdjacentTarget(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 5, 5
	book := &Item{Def: &content.ItemDef{Name: "book of Deathspell", Kind: "spellbook", Cost: 1, Deathspell: true}}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{5, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.CastSpellAt(book, rat.Pos) // too far: touch range only
	if g.EP != 5 || g.Dead || rat.HP != 9 {
		t.Error("a non-adjacent target refuses free (C: 'No one is there!', no turn, no EP)")
	}
	g.CastSpellAt(book, Pos{2, 1}) // empty tile
	if g.EP != 5 || !hasMessage(g, "No one is there!") {
		t.Errorf("an empty tile refuses with the C line, got %v", g.Messages)
	}
}
