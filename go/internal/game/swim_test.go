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
	if g.DeathCause != "drowning" {
		t.Errorf("morgue cause should be %q, got %q", "drowning", g.DeathCause)
	}
	if !hasMessage(g, "You drown...") {
		t.Errorf("expected the C drowning message, got %v", g.Messages)
	}
}

func TestFloatingPlayerCrossesWaterUnharmed(t *testing.T) {
	g := waterGame()
	g.AddEffect("levitate", 20)
	g.PlayerStep(DirE) // onto the water, airborne
	if g.Player != (Pos{2, 1}) {
		t.Fatalf("a floating player crosses deep water (C move_creature), at %v", g.Player)
	}
	if g.PlayerHP != 20 {
		t.Errorf("no swim fatigue while airborne (C swim() skips floaters): HP %d", g.PlayerHP)
	}
	g.PlayerStep(DirE) // and off the far side
	if g.Player != (Pos{3, 1}) || g.PlayerHP != 20 {
		t.Errorf("crossing should finish dry: at %v with HP %d", g.Player, g.PlayerHP)
	}
}

func TestFloatingPlayerSkipsTraps(t *testing.T) {
	g := combatGame()
	trap := &content.TileDef{ID: "dart_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
	g.Level.Set(Pos{2, 1}, trap)
	g.AddEffect("levitate", 20)
	g.PlayerStep(DirE)
	if g.HasEffect("poison") {
		t.Error("a floater glides over floor traps (C activate_trap)")
	}
}

func TestLandingInWaterPlunges(t *testing.T) {
	g := waterGame()
	g.AddEffect("levitate", 2) // expires right above the pool
	g.PlayerStep(DirE)         // onto the water (levitate 2->1 this turn)
	g.advanceWorld()           // levitate 1->0: the landing
	if !hasMessage(g, "You plunge into water!") {
		t.Fatalf("expected the C plunge message, got %v", g.Messages)
	}
	hp := g.PlayerHP
	g.advanceWorld() // now swimming: the drowning clock runs
	if g.PlayerHP != hp-1 {
		t.Errorf("after plunging the swim clock resumes: want HP %d, got %d", hp-1, g.PlayerHP)
	}
}

func TestLandingOnGround(t *testing.T) {
	g := combatGame()
	g.AddEffect("levitate", 1)
	g.PlayerStep(DirE) // levitate expires over plain floor
	if !hasMessage(g, "You land on the ground.") {
		t.Errorf("expected the C ground message, got %v", g.Messages)
	}
}

func TestLandingOnTrapSpringsIt(t *testing.T) {
	g := combatGame()
	trap := &content.TileDef{ID: "dart_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5}
	g.Level.Set(Pos{2, 1}, trap)
	g.AddEffect("levitate", 2)
	g.PlayerStep(DirE) // float onto the trap (no trigger; levitate 2->1)
	g.advanceWorld()   // expiry: land right on it
	if !g.HasEffect("poison") {
		t.Error("landing on a trap springs it (C change_altitude)")
	}
}
