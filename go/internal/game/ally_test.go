package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

// allyImp drops a player-allied imp into g at p.
func allyImp(g *Game, p Pos) *Creature {
	imp := &Creature{Def: &content.MonsterDef{ID: "imp", Name: "imp", Glyph: "i", HP: 6, Attack: 1000, Dodge: 2, Damage: "1d2"}, Pos: p, HP: 6, Ally: true}
	g.Level.Creatures = append(g.Level.Creatures, imp)
	return imp
}

func TestAllyBitesAdjacentHostile(t *testing.T) {
	g := combatGame()
	allyImp(g, Pos{3, 1})
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{4, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	hp := g.PlayerHP
	g.worldTick()
	if rat.HP >= 9 {
		t.Error("an ally fights the player's enemies (C charm/ai_offensive)")
	}
	if g.PlayerHP != hp {
		t.Error("the ally must not maul its master")
	}
}

func TestAllyPursuesHostileInRange(t *testing.T) {
	g := combatGame()
	imp := allyImp(g, Pos{3, 1})
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 0, Damage: "1d1"}, Pos: Pos{7, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.worldTick()
	if imp.Pos.X != 4 {
		t.Errorf("the ally should close on the rat, at %v", imp.Pos)
	}
}

func TestIdleAllyHeelsToPlayer(t *testing.T) {
	g := combatGame()
	imp := allyImp(g, Pos{8, 1}) // far from the player at (1,1), no hostiles
	g.worldTick()
	if imp.Pos.X != 7 {
		t.Errorf("an idle ally follows its master, at %v", imp.Pos)
	}
}

func TestSummonLifetimeExpires(t *testing.T) {
	g := combatGame()
	imp := allyImp(g, Pos{2, 1})
	imp.Lifetime = 2
	g.worldTick()
	g.worldTick()
	if g.Level.CreatureAt(Pos{2, 1}) != nil {
		t.Fatal("the summon should disappear when its lifetime runs out (C game.c)")
	}
	if !hasMessage(g, "disappears.") {
		t.Errorf("expected the C disappearance message, got %v", g.Messages)
	}
	for _, it := range g.Level.Items {
		if it.Pos == imp.Pos {
			t.Error("a timed-out summon leaves no corpse")
		}
	}
}

func TestHostilePrefersAdjacentAlly(t *testing.T) {
	g := combatGame()
	imp := allyImp(g, Pos{3, 1})
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 1000, Dodge: 0, Damage: "1d2"}, Pos: Pos{4, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.Player = Pos{1, 1} // two away from the rat; the imp is the adjacent target
	hp := g.PlayerHP
	g.worldTick()
	if imp.HP >= 6 {
		t.Error("a hostile bites the ally at its flank first (C enemies split)")
	}
	if g.PlayerHP != hp {
		t.Error("the player was out of reach this tick")
	}
}

func TestSummonAllyArrivesBesideThePlayer(t *testing.T) {
	g := combatGame()
	g.Content.Monsters = map[string]*content.MonsterDef{"imp": {ID: "imp", Name: "imp", Glyph: "i", HP: 6, Attack: 8, Dodge: 2, Damage: "1d4"}}
	if !g.SummonAlly("imp", 500) {
		t.Fatal("the summon should find a spot")
	}
	var imp *Creature
	for _, c := range g.Level.Creatures {
		if c.Def.ID == "imp" {
			imp = c
		}
	}
	if imp == nil || !imp.Ally || imp.Lifetime != 500 {
		t.Fatalf("expected a charmed 500-tick imp, got %+v", imp)
	}
	if d := chebyshev(imp.Pos, g.Player); d != 1 {
		t.Errorf("the familiar arrives at its master's side, %d away", d)
	}
	if g.SummonAlly("ghost", 1) {
		t.Error("an unknown def must not summon")
	}
}
