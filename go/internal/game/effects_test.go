package game

import "testing"

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
