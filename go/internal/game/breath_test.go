package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestBreathConeShape(t *testing.T) {
	g := combatGame() // 10x3, all floor
	tiles := g.breathCone(Pos{1, 1}, 1, 0, 3)
	want := map[Pos]bool{
		{2, 0}: true, {2, 1}: true, {2, 2}: true, // step 1, swell 1
		{3, 0}: true, {3, 1}: true, {3, 2}: true, // step 2, swell 1
		{4, 1}: true, // step 3: the tip narrows (C swell=MIN(range,swell))
	}
	if len(tiles) != len(want) {
		t.Fatalf("cone east range 3: want %d tiles, got %d (%v)", len(want), len(tiles), tiles)
	}
	for _, p := range tiles {
		if !want[p] {
			t.Errorf("unexpected cone tile %v", p)
		}
	}
}

func TestBreathConeStopsAtWalls(t *testing.T) {
	g := combatGame()
	wall := &content.TileDef{ID: "wall", Glyph: "#", Passable: false}
	g.Level.Set(Pos{3, 1}, wall) // block the spine at step 2
	tiles := g.breathCone(Pos{1, 1}, 1, 0, 3)
	for _, p := range tiles {
		if p.X > 2 && p.Y == 1 {
			t.Errorf("the cone spine must stop at the wall, got %v", p)
		}
	}
}

func TestFireBreathBurnsTheCone(t *testing.T) {
	g := combatGame()
	dragon := &Creature{Def: &content.MonsterDef{ID: "dragon", Name: "dragon", Glyph: "D", HP: 60, Attack: 10, Dodge: 2, Damage: "2d6", Breath: "fire"}, Pos: Pos{1, 1}, HP: 60}
	bystander := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{3, 0}, HP: 9}
	far := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, dragon, bystander, far)
	g.Player = Pos{4, 1} // three east of the dragon: the cone's tip
	g.worldTick()
	if g.PlayerHP >= 20 {
		t.Error("the breath should burn the player at the cone's tip (C 2d4, no dodge)")
	}
	if bystander.HP >= 9 {
		t.Error("friendly fire: a bystander inside the cone burns too (C breath_weapon)")
	}
	if far.HP != 9 {
		t.Error("a creature outside the cone is untouched")
	}
	if !hasMessage(g, "The dragon breathes fire!") {
		t.Errorf("expected the C breath line, got %v", g.Messages)
	}
}

func TestPoisonBreathPoisons(t *testing.T) {
	g := combatGame()
	toad := &Creature{Def: &content.MonsterDef{ID: "toad", Name: "giant slimy toad", Glyph: "t", HP: 14, Attack: 5, Dodge: 1, Damage: "1d4", Breath: "poison"}, Pos: Pos{1, 1}, HP: 14}
	g.Level.Creatures = append(g.Level.Creatures, toad)
	g.Player = Pos{4, 1}
	g.worldTick()
	if !g.HasEffect("poison") {
		t.Error("noxious breath poisons the player for 12 turns (C NOXIOUS_BREATH_POISON)")
	}
	if !hasMessage(g, "You inhale the vile fumes!") {
		t.Errorf("expected the C inhale line, got %v", g.Messages)
	}
}

func TestBreatherClosesWhenOutOfRange(t *testing.T) {
	g := combatGame()
	dragon := &Creature{Def: &content.MonsterDef{ID: "dragon", Name: "dragon", Glyph: "D", HP: 60, Attack: 10, Dodge: 2, Damage: "2d6", Breath: "fire"}, Pos: Pos{8, 1}, HP: 60}
	g.Level.Creatures = append(g.Level.Creatures, dragon)
	g.worldTick()
	if dragon.Pos.X != 7 {
		t.Errorf("out of breath range the dragon closes in, at %v", dragon.Pos)
	}
	if g.PlayerHP != 20 {
		t.Error("no breath should reach from range 7")
	}
}

func TestCastBreatheFireBurnsTheCone(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 10, 10
	book := &Item{Def: &content.ItemDef{Name: "book of Breathe Fire", Kind: "spellbook", Cost: 5, Breath: "fire"}}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{4, 1}, HP: 9}
	flank := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{3, 0}, HP: 9}
	far := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat, flank, far)
	g.CastSpellAt(book, rat.Pos)
	if rat.HP >= 9 || flank.HP >= 9 {
		t.Error("the cone should burn the target and the flank bystander")
	}
	if far.HP != 9 {
		t.Error("a creature beyond the cone is untouched")
	}
	if g.EP != 5 {
		t.Errorf("the cast costs 5 EP (C attrs.c), EP %d", g.EP)
	}
	if !hasMessage(g, "You breathe fire!") {
		t.Errorf("expected the C cast line, got %v", g.Messages)
	}
}

func TestCastNoxiousBreathPoisons(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 10, 10
	book := &Item{Def: &content.ItemDef{Name: "book of Noxious Breath", Kind: "spellbook", Cost: 5, Breath: "poison"}}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{3, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.CastSpellAt(book, rat.Pos)
	if !rat.HasEffect("poison") {
		t.Error("noxious breath poisons the cone's occupants (C NOXIOUS_BREATH_POISON)")
	}
	if !hasMessage(g, "You breathe poison!") {
		t.Errorf("expected the C cast line, got %v", g.Messages)
	}
}

func TestBreathCastRefusedWithoutEP(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 2, 10
	book := &Item{Def: &content.ItemDef{Name: "book of Breathe Fire", Kind: "spellbook", Cost: 5, Breath: "fire"}}
	g.CastSpellAt(book, Pos{4, 1})
	if g.EP != 2 {
		t.Error("a refused cast must not spend EP")
	}
	if hasMessage(g, "You breathe fire!") {
		t.Error("no EP, no fire")
	}
}
