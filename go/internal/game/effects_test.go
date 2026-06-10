package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestPoisonDamagesAndKills(t *testing.T) {
	g := &Game{PlayerHP: 3, PlayerMax: 20}
	g.AddEffect("poison", 5)
	g.tickEffects() // 3 -> 2
	if g.PlayerHP != 2 {
		t.Errorf("HP = %d, want 2 after one poison tick", g.PlayerHP)
	}
	for i := 0; i < 5 && !g.Dead; i++ {
		g.tickEffects()
	}
	if !g.Dead || g.DeathCause != "poison" {
		t.Errorf("poison should kill (dead=%v cause=%q)", g.Dead, g.DeathCause)
	}
}

func TestRegenHealsClampsExpires(t *testing.T) {
	g := &Game{PlayerHP: 18, PlayerMax: 20}
	g.AddEffect("regen", 3)
	g.tickEffects() // 18 -> 19
	g.tickEffects() // 19 -> 20
	g.tickEffects() // clamp at 20; effect expires
	if g.PlayerHP != 20 {
		t.Errorf("HP = %d, want 20 (clamped)", g.PlayerHP)
	}
	if len(g.Effects) != 0 {
		t.Errorf("regen should have expired, effects = %v", g.Effects)
	}
}

func TestAddEffectRefreshesToLonger(t *testing.T) {
	g := &Game{PlayerHP: 10, PlayerMax: 10}
	g.AddEffect("poison", 3)
	g.AddEffect("poison", 6) // refresh to the longer duration
	if len(g.Effects) != 1 || g.Effects[0].Turns != 6 {
		t.Errorf("expected one poison effect with 6 turns, got %v", g.Effects)
	}
	g.AddEffect("poison", 2) // shorter — no change
	if g.Effects[0].Turns != 6 {
		t.Errorf("shorter refresh should not shorten, got %d", g.Effects[0].Turns)
	}
}

func TestEffectsSummary(t *testing.T) {
	g := &Game{PlayerHP: 10, PlayerMax: 10}
	g.AddEffect("poison", 3)
	g.AddEffect("regen", 3)
	if got := g.EffectsSummary(); got != "Poisoned, Regenerating" {
		t.Errorf("summary = %q, want \"Poisoned, Regenerating\"", got)
	}
}

// Poison + regen must net out cleanly and never leave Dead set with HP > 0
// (order-independent death resolution).
func TestPoisonAndRegenNetOut(t *testing.T) {
	g := &Game{PlayerHP: 1, PlayerMax: 20}
	g.AddEffect("poison", 3)
	g.AddEffect("regen", 3)
	g.tickEffects() // -1 +1 = net 0
	if g.Dead || g.PlayerHP != 1 {
		t.Errorf("expected alive at HP 1, got dead=%v hp=%d", g.Dead, g.PlayerHP)
	}
}

func TestAddEffectRejectsBadInput(t *testing.T) {
	g := &Game{PlayerHP: 10, PlayerMax: 10}
	g.AddEffect("", 5)        // empty kind
	g.AddEffect("poison", 0)  // non-positive duration
	g.AddEffect("poison", -3) // negative duration
	if len(g.Effects) != 0 {
		t.Errorf("bad inputs should add no effects, got %v", g.Effects)
	}
}

// Creatures carry the same timed effects as the player; poison whittles their
// HP and resolves death through killCreature (corpse drop + removal).
func TestCreaturePoisonDamagesAndKills(t *testing.T) {
	g := combatGame()
	m := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3, Damage: "1d1"}, Pos: Pos{3, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, m)
	m.AddEffect("poison", 5)

	if g.tickCreatureEffects(m) { // 3 -> 2
		t.Fatal("one tick should not kill a 3-HP creature")
	}
	if m.HP != 2 {
		t.Errorf("HP = %d, want 2 after one poison tick", m.HP)
	}
	died := false
	for i := 0; i < 5 && !died; i++ {
		died = g.tickCreatureEffects(m)
	}
	if !died {
		t.Fatal("poison should eventually kill the creature")
	}
	if g.Level.CreatureAt(Pos{3, 1}) != nil {
		t.Error("a creature poisoned to death should be removed from the level")
	}
}

func TestCreatureAddEffectRefreshesToLonger(t *testing.T) {
	m := &Creature{Def: &content.MonsterDef{ID: "rat"}, HP: 5}
	m.AddEffect("poison", 3)
	m.AddEffect("poison", 6) // refresh to the longer duration
	if len(m.Effects) != 1 || m.Effects[0].Turns != 6 {
		t.Errorf("expected one poison effect with 6 turns, got %v", m.Effects)
	}
	m.AddEffect("poison", 2) // shorter — no change
	if m.Effects[0].Turns != 6 {
		t.Errorf("shorter refresh should not shorten, got %d", m.Effects[0].Turns)
	}
}

func TestRemoveEffectDropsOnlyNamedKind(t *testing.T) {
	g := &Game{}
	g.AddEffect("slow", 10)
	g.AddEffect("poison", 5)
	g.RemoveEffect("slow")
	if g.HasEffect("slow") {
		t.Error("slow should be gone after RemoveEffect")
	}
	if !g.HasEffect("poison") {
		t.Error("poison should be untouched by removing slow")
	}
	g.RemoveEffect("haste") // absent kind: a no-op
	if len(g.Effects) != 1 {
		t.Errorf("expected 1 effect left, got %v", g.Effects)
	}
}
