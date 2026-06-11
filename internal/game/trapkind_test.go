package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

func plantTrap(g *Game, p Pos, def *content.TileDef) *Tile {
	g.Level.Set(p, def)
	t := g.Level.At(p)
	t.Disguise = testFloorDef
	t.TrapDifficulty = 12
	return t
}

func TestWebTrapSticksAndStruggles(t *testing.T) {
	g := combatGame()
	plantTrap(g, Pos{2, 1}, &content.TileDef{ID: "web_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "web", EffectTurns: 27})
	g.PlayerStep(DirE)
	if !g.HasEffect("web") || !hasMessage(g, "You get stuck in a web!") {
		t.Fatalf("the web should stick with the C line, got %v", g.Messages)
	}
	before := g.Player
	g.PlayerStep(DirE) // a move attempt becomes a struggle: -6 from the clock
	if g.Player != before {
		t.Error("a webbed player does not move")
	}
	if !hasMessage(g, "You struggle in the web!") {
		t.Errorf("expected the struggle line, got %v", g.Messages)
	}
	// 27 - tick - 6... struggle repeatedly until free.
	for i := 0; i < 6; i++ {
		g.PlayerStep(DirE)
	}
	if g.HasEffect("web") {
		t.Error("six struggles should tear through 27 turns of web")
	}
	if !hasMessage(g, "You break free of the web.") {
		t.Errorf("expected the C break-free line, got %v", g.Messages)
	}
}

func TestWebbedMonsterHolds(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 1000, Dodge: 0, Damage: "1d2"}, Pos: Pos{2, 1}, HP: 9}
	rat.AddEffect("web", 27)
	g.Level.Creatures = append(g.Level.Creatures, rat)
	hp := g.PlayerHP
	g.worldTick()
	if g.PlayerHP != hp || rat.Pos != (Pos{2, 1}) {
		t.Error("a webbed monster struggles instead of acting")
	}
	for i := 0; i < 6; i++ {
		g.worldTick()
	}
	if rat.HasEffect("web") {
		t.Error("the monster should eventually tear free")
	}
}

func TestFlashTrapBlinds(t *testing.T) {
	g := combatGame()
	plantTrap(g, Pos{2, 1}, &content.TileDef{ID: "flash_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "blind", EffectTurns: 33})
	g.PlayerStep(DirE)
	if !g.HasEffect("blind") || !hasMessage(g, "You are blinded by a bright flash!") {
		t.Errorf("the flash should blind with the C line, got %v", g.Messages)
	}
}

func TestPlateTrapShocks(t *testing.T) {
	g := combatGame()
	plantTrap(g, Pos{2, 1}, &content.TileDef{ID: "plate_trap", Glyph: "^", Passable: true, Transparent: true, Damage: "1d4+1"})
	g.PlayerStep(DirE)
	if g.PlayerHP >= 20 {
		t.Error("the plate should shock for 1d4+1 (C PLATE_DAMAGE)")
	}
	if !hasMessage(g, "You step on an electrified plate!") {
		t.Errorf("expected the C plate line, got %v", g.Messages)
	}
}

func TestPolymorphTrapTransforms(t *testing.T) {
	g := combatGame()
	g.Content.Monsters = map[string]*content.MonsterDef{"rat": {ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 2, Dodge: 1, Damage: "1d2"}}
	plantTrap(g, Pos{2, 1}, &content.TileDef{ID: "polymorph_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "polymorph", EffectTurns: 85})
	g.PlayerStep(DirE)
	if g.Shape == nil || !g.HasEffect("polymorph") {
		t.Error("the trap should transform like the potion (C shapeshift_random)")
	}
	if !hasMessage(g, "You step on a polymorph trap!") {
		t.Errorf("expected the C trap line, got %v", g.Messages)
	}
}
