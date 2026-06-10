package game

import "strings"

// Effect is a timed status effect on the player.
type Effect struct {
	Kind  string
	Turns int
}

// effectLabels maps effect kinds to their HUD labels.
var effectLabels = map[string]string{
	"poison":  "Poisoned",
	"regen":   "Regenerating",
	"slow":    "Slowed",
	"blind":   "Blinded",
	"confuse": "Confused",
	"fear":    "Afraid",
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
// expires them. Called once per player action (from monstersAct).
func (g *Game) tickEffects() {
	if len(g.Effects) == 0 {
		return
	}
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
		}
	}
	g.Effects = kept
	// Resolve death once, after the net HP change of this turn, so a regen can
	// offset poison instead of leaving Dead set while HP is still positive.
	if g.PlayerHP <= 0 && !g.Dead {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = "poison"
		g.log("The poison overcomes you. You die.")
	}
}

// EffectsSummary is a comma-separated list of active effect labels for the HUD,
// or "" when there are none.
func (g *Game) EffectsSummary() string {
	if len(g.Effects) == 0 {
		return ""
	}
	labels := make([]string, 0, len(g.Effects))
	for _, e := range g.Effects {
		if l := effectLabels[e.Kind]; l != "" {
			labels = append(labels, l)
		} else {
			labels = append(labels, e.Kind)
		}
	}
	return strings.Join(labels, ", ")
}
