package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

// hiddenMimic plants a disguised mimic at p, posing as a potion.
func hiddenMimic(g *Game, p Pos) *Creature {
	m := &Creature{
		Def: &content.MonsterDef{ID: "mimic", Name: "mimic", Glyph: "m", HP: 5, Attack: 6, Dodge: 0, Damage: "1d4", Mimic: true},
		Pos: p, HP: 5,
		Disguised:  true,
		DisguiseAs: &content.ItemDef{ID: "healing_potion", Name: "healing potion", Glyph: "!", Color: content.ColorRed, Kind: "potion"},
	}
	g.Level.Creatures = append(g.Level.Creatures, m)
	return m
}

func TestDisguisedMimicDoesNothing(t *testing.T) {
	g := combatGame()
	hiddenMimic(g, Pos{2, 1}) // adjacent to the player
	hp := g.PlayerHP
	for i := 0; i < 5; i++ {
		g.worldTick()
	}
	if g.PlayerHP != hp {
		t.Error("a disguised mimic does nothing at all (C ai_mimic)")
	}
}

func TestFirstSwingRevealsAndIsWasted(t *testing.T) {
	g := combatGame()
	m := hiddenMimic(g, Pos{2, 1})
	g.PlayerStep(DirE) // the bump that finds out
	if m.Disguised {
		t.Fatal("the swing should reveal the mimic")
	}
	if m.HP != 5 {
		t.Error("the revealing swing is wasted — no damage (C combat.c:237)")
	}
	if !hasMessage(g, "Wait! That is a small mimic!") {
		t.Errorf("expected the C reveal line, got %v", g.Messages)
	}
	g.PlayerStep(DirE) // now it's just a monster
	if m.HP >= 5 && g.Level.CreatureAt(Pos{2, 1}) != nil {
		t.Error("the second swing lands normally")
	}
}

func TestRevealedMimicBitesButNeverMoves(t *testing.T) {
	g := combatGame()
	m := hiddenMimic(g, Pos{2, 1})
	m.Disguised = false
	m.Def.Attack = 1000
	hp := g.PlayerHP
	g.worldTick()
	if g.PlayerHP >= hp {
		t.Error("a revealed mimic bites the adjacent player")
	}
	g.Player = Pos{6, 1} // walk away: it cannot follow
	for i := 0; i < 5; i++ {
		g.worldTick()
	}
	if m.Pos != (Pos{2, 1}) {
		t.Errorf("a mimic is rooted (C attr_p_move), at %v", m.Pos)
	}
}

func TestWandDamageRevealsMimic(t *testing.T) {
	g := combatGame()
	m := hiddenMimic(g, Pos{3, 1})
	wand := &Item{Def: &content.ItemDef{ID: "wand_force_bolt", Name: "wand of force bolt", Kind: "wand", Damage: "1d1"}, Charges: 2}
	g.ZapWand(wand, m.Pos)
	if m.Disguised {
		t.Error("a bolt to the glamour exposes it (C reveal_mimic in missile paths)")
	}
}
