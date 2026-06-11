package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

// schedulerGame is a combat corridor with a pursuing rat seven tiles east, so
// tick counts are observable as tiles closed.
func schedulerGame() (*Game, *Creature) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	return g, rat
}

func TestSchedulerOneTickPerTurnAtBaseSpeed(t *testing.T) {
	g, rat := schedulerGame()
	g.advanceWorld()
	if rat.Pos.X != 7 {
		t.Errorf("at base speed one turn is one tick: rat should be at x=7, got x=%d", rat.Pos.X)
	}
}

func TestSlowedPlayerGivesMonstersTwoTicks(t *testing.T) {
	g, rat := schedulerGame()
	g.AddEffect("slow", 10)
	g.advanceWorld()
	if rat.Pos.X != 6 {
		t.Errorf("a slowed player owes two ticks per turn: rat should be at x=6, got x=%d", rat.Pos.X)
	}
}

func TestHastedPlayerGetsFreeAction(t *testing.T) {
	g, rat := schedulerGame()
	g.AddEffect("haste", 20)
	for i := 0; i < 4; i++ {
		g.advanceWorld()
	}
	if rat.Pos.X != 4 {
		t.Fatalf("first four hasted turns each pass a tick: rat should be at x=4, got x=%d", rat.Pos.X)
	}
	g.advanceWorld() // banked surplus covers this turn: no tick passes
	if rat.Pos.X != 4 {
		t.Errorf("fifth hasted turn is free: rat should still be at x=4, got x=%d", rat.Pos.X)
	}
	g.advanceWorld()
	if rat.Pos.X != 3 {
		t.Errorf("sixth turn ticks again: rat should be at x=3, got x=%d", rat.Pos.X)
	}
}

func TestPlayerEffectsBurnPerTurnNotPerTick(t *testing.T) {
	g, _ := schedulerGame()
	g.AddEffect("slow", 10)
	g.AddEffect("poison", 3)
	g.advanceWorld() // two world ticks pass, but the player's own clocks run once
	if g.PlayerHP != 19 {
		t.Errorf("poison burns once per player turn even when slowed: want HP 19, got %d", g.PlayerHP)
	}
}

func TestHasteHUDLabel(t *testing.T) {
	g := combatGame()
	g.AddEffect("haste", 5)
	if got := g.EffectsSummary(); got != "Hastened" {
		t.Errorf("EffectsSummary = %q, want %q", got, "Hastened")
	}
}
