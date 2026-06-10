package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

// waterGame is a corridor with a water tile directly east of the player.
func waterGame() *Game {
	g := combatGame()
	water := &content.TileDef{ID: "water", Glyph: "_", Passable: false, Transparent: true, Water: true}
	g.Level.Set(Pos{2, 1}, water)
	return g
}

func TestSightedPlayerCannotWalkIntoWater(t *testing.T) {
	g := waterGame()
	g.PlayerStep(DirE)
	if g.Player != (Pos{1, 1}) {
		t.Errorf("a sighted walker can't enter water (C move_creature), player at %v", g.Player)
	}
}

func TestBlindPlayerStumblesIntoWater(t *testing.T) {
	g := waterGame()
	g.AddEffect("blind", 10)
	g.PlayerStep(DirE)
	if g.Player != (Pos{2, 1}) {
		t.Errorf("a blinded walker blunders into water (C move_creature), player at %v", g.Player)
	}
}

func TestStandingInWaterDrains1HPPerTurn(t *testing.T) {
	g := waterGame()
	g.AddEffect("blind", 2) // long enough to stumble in, short enough to expire
	g.PlayerStep(DirE)      // into the water; passTurn runs the first swim check
	if g.PlayerHP != 19 {
		t.Fatalf("first turn in water already exceeds swimming 0: want HP 19, got %d", g.PlayerHP)
	}
	g.advanceWorld() // treading water another turn
	if g.PlayerHP != 18 {
		t.Errorf("each turn under water costs 1 HP: want 18, got %d", g.PlayerHP)
	}
}

func TestWadingOutResetsSwimFatigue(t *testing.T) {
	g := waterGame()
	g.AddEffect("blind", 2)
	g.PlayerStep(DirE) // in: HP 19, fatigue 1
	g.PlayerStep(DirW) // back onto floor: fatigue resets, no further damage
	if g.PlayerHP != 19 {
		t.Fatalf("dry land stops the drain: want HP 19, got %d", g.PlayerHP)
	}
	if g.swimFatigue != 0 {
		t.Errorf("fatigue resets on land (C swim()), got %d", g.swimFatigue)
	}
}

func TestPlayerDrowns(t *testing.T) {
	g := waterGame()
	g.PlayerHP = 1
	g.AddEffect("blind", 10)
	g.PlayerStep(DirE)
	if !g.Dead {
		t.Fatal("at 1 HP the first turn under water drowns the player")
	}
	if g.DeathCause != "drowned" {
		t.Errorf("morgue cause should be %q, got %q", "drowned", g.DeathCause)
	}
	if !hasMessage(g, "You drown...") {
		t.Errorf("expected the C drowning message, got %v", g.Messages)
	}
}
