package game

import "strings"

// Effect is a timed status effect on the player.
type Effect struct {
	Kind  string
	Turns int
}

// effectLabels maps effect kinds to their HUD labels.
var effectLabels = map[string]string{
	"poison":      "Poisoned",
	"regen":       "Regenerating",
	"slow":        "Slowed",
	"haste":       "Hastened",
	"blind":       "Blinded",
	"confuse":     "Confused",
	"fear":        "Afraid",
	"sleep":       "Asleep",
	"levitate":    "Floating",
	"flame_hands": "Flaming hands",
}

// HasEffect reports whether a timed effect of the given kind is active on the
// player.
func (g *Game) HasEffect(kind string) bool {
	for _, e := range g.Effects {
		if e.Kind == kind {
			return true
		}
	}
	return false
}

// effectVerbs phrases how a source inflicts an effect, for the zap message.
var effectVerbs = map[string]string{
	"poison":  "poisons",
	"slow":    "slows",
	"confuse": "confuses",
	"fear":    "frightens",
}

// effectVerb returns the verb describing inflicting kind, with a generic
// fallback for effects that have no specific phrasing.
func effectVerb(kind string) string {
	if v := effectVerbs[kind]; v != "" {
		return v
	}
	return "afflicts"
}

// addEffect adds or refreshes a timed effect in effects and returns the updated
// slice. An already-active effect of the same kind is kept at the longer of the
// two durations. Empty kinds and non-positive durations are ignored. Shared by
// the player and creatures so both get identical stacking semantics.
func addEffect(effects []Effect, kind string, turns int) []Effect {
	if kind == "" || turns <= 0 {
		return effects
	}
	for i := range effects {
		if effects[i].Kind == kind {
			if turns > effects[i].Turns {
				effects[i].Turns = turns
			}
			return effects
		}
	}
	return append(effects, Effect{Kind: kind, Turns: turns})
}

// AddEffect applies a status effect to the player for the given number of turns,
// refreshing an already-active effect of the same kind to the longer duration.
func (g *Game) AddEffect(kind string, turns int) {
	g.Effects = addEffect(g.Effects, kind, turns)
}

// RemoveEffect drops an active player effect by kind (the C's effect_expire,
// e.g. a potion of speed cancelling slow); an absent kind is a no-op.
func (g *Game) RemoveEffect(kind string) {
	for i, e := range g.Effects {
		if e.Kind == kind {
			g.Effects = append(g.Effects[:i], g.Effects[i+1:]...)
			return
		}
	}
}

// tickCreatureEffects applies each of a creature's active effects (poison costs
// it 1 HP), decrements and expires them, and resolves death through killCreature
// when its HP reaches 0. It reports whether the creature died this tick.
func (g *Game) tickCreatureEffects(m *Creature) bool {
	if len(m.Effects) == 0 {
		return false
	}
	kept := m.Effects[:0]
	for _, e := range m.Effects {
		if e.Kind == "poison" {
			m.HP--
		}
		e.Turns--
		if e.Turns > 0 {
			kept = append(kept, e)
		}
	}
	m.Effects = kept
	if m.HP <= 0 {
		g.killCreature(m)
		return true
	}
	return false
}

// tickEffects applies each active effect's per-turn outcome, then decrements and
// expires them. Called once per player turn (from advanceWorld).
func (g *Game) tickEffects() {
	if len(g.Effects) == 0 {
		return
	}
	landed := false
	kept := g.Effects[:0]
	for _, e := range g.Effects {
		switch e.Kind {
		case "poison":
			g.PlayerHP--
		case "regen":
			if g.PlayerHP < g.PlayerMax {
				g.PlayerHP++
			}
		}
		e.Turns--
		if e.Turns > 0 {
			kept = append(kept, e)
		} else if e.Kind == "sleep" {
			g.log("You wake up!") // sleep ran its course (C creature_sleep)
		} else if e.Kind == "levitate" {
			landed = true // resolved below, once the effects slice is settled
		}
	}
	g.Effects = kept
	if landed {
		g.land()
	}
	// Resolve death once, after the net HP change of this turn, so a regen can
	// offset poison instead of leaving Dead set while HP is still positive.
	if g.PlayerHP <= 0 && !g.Dead {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = "poison"
		g.log("The poison overcomes you. You die.")
	}
}

// land resolves the end of levitation (C change_altitude): into deep water the
// swim clock takes over; a trap below springs on touchdown; otherwise it's
// just solid ground.
func (g *Game) land() {
	tile := g.Level.At(g.Player).Def
	if tile.Water {
		g.log("You plunge into water!")
		return
	}
	if tile.Lava {
		g.log("You land in lava!")
		g.lavaCheck() // the bath is immediate (C altitude.c:78)
		return
	}
	g.log("You land on the ground.")
	if tile.Effect != "" {
		g.springTrapAt(g.Player)
	}
}

// DispelLevitation cuts levitation short and runs the landing (the C elixir's
// set_attr(levitate,0) + change_altitude). A grounded player is a no-op.
func (g *Game) DispelLevitation() {
	if !g.HasEffect("levitate") {
		return
	}
	g.RemoveEffect("levitate")
	g.land()
}

// EffectsSummary is a comma-separated list of active effect labels for the HUD
// — plus the burden flag, which is carried state rather than a timed effect —
// or "" when there is nothing to show.
func (g *Game) EffectsSummary() string {
	labels := make([]string, 0, len(g.Effects)+1)
	for _, e := range g.Effects {
		if l := effectLabels[e.Kind]; l != "" {
			labels = append(labels, l)
		} else {
			labels = append(labels, e.Kind)
		}
	}
	if g.burdened() {
		labels = append(labels, "Burdened")
	}
	return strings.Join(labels, ", ")
}
