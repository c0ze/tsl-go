package game

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func hasMessage(g *Game, want string) bool {
	for _, m := range g.Messages {
		if strings.Contains(m, want) {
			return true
		}
	}
	return false
}

func TestSleepingPlayerLosesTurns(t *testing.T) {
	g, rat := schedulerGame()
	g.AddEffect("sleep", 3)
	g.advanceWorld() // the turn sleep took hold; the slept turns run right after
	if rat.Pos.X != 5 {
		t.Errorf("three turns pass while the player sleeps: rat should be at x=5, got x=%d", rat.Pos.X)
	}
	if g.HasEffect("sleep") {
		t.Error("sleep should have run its course")
	}
	if !hasMessage(g, "You wake up!") {
		t.Errorf("expected the C wake message, got %v", g.Messages)
	}
}

func TestDamageWakesSleepingPlayer(t *testing.T) {
	g := combatGame()
	ogre := &Creature{Def: &content.MonsterDef{ID: "ogre", Name: "ogre", Glyph: "O", HP: 99, Attack: 1000, Dodge: 0, Damage: "1d2"}, Pos: Pos{2, 1}, HP: 99}
	g.Level.Creatures = append(g.Level.Creatures, ogre)
	g.AddEffect("sleep", 25)
	g.advanceWorld()
	if g.HasEffect("sleep") {
		t.Error("the ogre's blow should have woken the player long before 25 turns")
	}
	if g.PlayerHP >= 20 {
		t.Errorf("the player should have been hit awake, HP still %d", g.PlayerHP)
	}
	if g.PlayerHP < 18 {
		t.Errorf("waking on the first hit should cost at most one 1d2 swing, HP %d", g.PlayerHP)
	}
	if !hasMessage(g, "You wake up!") {
		t.Errorf("expected the C wake message, got %v", g.Messages)
	}
}

func TestSleepingMonsterHoldsUntilStruck(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 1000, Dodge: 0, Damage: "5d6"}, Pos: Pos{2, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	rat.AddEffect("sleep", 50)
	hp := g.PlayerHP
	g.worldTick()
	if g.PlayerHP != hp {
		t.Error("a sleeping monster must not attack")
	}
	if rat.Pos != (Pos{2, 1}) {
		t.Error("a sleeping monster must not move")
	}
	g.PlayerStep(DirE) // strike it: a landed hit wakes it
	if !g.Dead && rat.HP < 9 && rat.HasEffect("sleep") {
		t.Error("a struck monster should wake (sleep effect removed)")
	}
}

func TestMissedSwingDoesNotWakeMonster(t *testing.T) {
	g := combatGame()
	// Dodge so high the player's swing is all but guaranteed to whiff; the C
	// wakes the sleeper only after a hit lands ("we *hit* before the enemy
	// wakes up", combat.c).
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 0, Dodge: 100000, Damage: "1d1"}, Pos: Pos{2, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	rat.AddEffect("sleep", 50)
	g.PlayerStep(DirE)
	if rat.HP == 9 && !rat.HasEffect("sleep") {
		t.Error("a missed swing must not wake a sleeping monster (C combat.c)")
	}
}
