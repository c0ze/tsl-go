package game

// Wounds (C wounds.c): some claws, fangs, and blades open a bleeding wound on
// a landed hit. A wounded creature loses 1 HP each time it moves until the
// wound closes (the effect runs out) or first aid stops it.

// meleeWoundTime is how many turns of bleeding one wounding hit adds (C
// rules.h MELEE_WOUND_TIME).
const meleeWoundTime = 12

// prolongEffect extends an active effect by turns, or starts it (C
// prolong_effect adds to the remaining time rather than keeping the longer).
func prolongEffect(effects []Effect, kind string, turns int) []Effect {
	for i := range effects {
		if effects[i].Kind == kind {
			effects[i].Turns += turns
			return effects
		}
	}
	return append(effects, Effect{Kind: kind, Turns: turns})
}

// rollWound reports whether a hit from a weapon with pct% wounding chance
// wounds (C combat.c: tslrnd() % 100 < attr_i_wound). Nothing is rolled for a
// weapon that can't wound, so the dice stream only moves when it matters.
func (g *Game) rollWound(pct int) bool {
	return pct > 0 && g.RNG.Intn(100) < pct
}

// woundPlayer opens (or deepens) a bleeding wound on the player.
func (g *Game) woundPlayer() {
	g.Effects = prolongEffect(g.Effects, "wound", meleeWoundTime)
	g.log("You have been wounded!")
}

// woundCreature opens a bleeding wound on m unless it is immune (slimes,
// flame spirits: C attr_wound_immunity).
func (g *Game) woundCreature(m *Creature) {
	if m.Def.WoundImmune {
		return
	}
	m.Effects = prolongEffect(m.Effects, "wound", meleeWoundTime)
	if g.canSee(m.Pos) {
		g.log("The %s is wounded!", m.Def.Name)
	}
}

// playerBleeds deals the wounded player's per-move blood loss (C
// wound_damage after move_creature).
func (g *Game) playerBleeds() {
	if !g.HasEffect("wound") || g.Dead {
		return
	}
	g.PlayerHP--
	g.Sound("hurt")
	g.log("You are bleeding!")
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = "bled to death"
		g.log("You have lost too much blood. You die.")
	}
}

// creatureBleeds is playerBleeds for a monster that just moved; one that
// bleeds out collapses (C wound_damage). It reports whether m died.
func (g *Game) creatureBleeds(m *Creature) bool {
	if !m.HasEffect("wound") {
		return false
	}
	m.HP--
	if m.HP > 0 {
		return false
	}
	// C can_see_creature: never while blind; else in view, or right beside you.
	if !g.playerBlinded() && (g.canSee(m.Pos) || chebyshev(m.Pos, g.Player) <= 1) {
		g.log("The %s collapses!", m.Def.Name)
	}
	g.dropCorpseAndRemove(m)
	return true
}

// canSee reports whether the player can currently see p (C can_see), which
// decides whether events there are narrated.
func (g *Game) canSee(p Pos) bool {
	return g.Level.InBounds(p) && g.Level.At(p).Visible
}

// FirstAid is the first-aid spell (C magic.c first_aid): it stops the
// bleeding and restores 1d3 HP. It reports false — "You are not wounded." —
// when there is nothing to treat, so the cast costs neither EP nor a turn.
func (g *Game) FirstAid() bool {
	if !g.HasEffect("wound") {
		g.log("You are not wounded.")
		return false
	}
	g.RemoveEffect("wound")
	g.log("You are no longer bleeding.")
	g.PlayerHP = min(g.PlayerHP+g.RNG.Roll(1, 3), g.PlayerMax)
	return true
}
