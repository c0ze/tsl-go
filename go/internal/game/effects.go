package game

import "strings"

// Effect is a timed status effect on the player.
type Effect struct {
	Kind  string
	Turns int
}

// effectLabels maps effect kinds to their HUD labels.
var effectLabels = map[string]string{
	"poison": "Poisoned",
	"regen":  "Regenerating",
}

// AddEffect applies a status effect for the given number of turns, refreshing an
// already-active effect of the same kind to the longer duration.
func (g *Game) AddEffect(kind string, turns int) {
	for i := range g.Effects {
		if g.Effects[i].Kind == kind {
			if turns > g.Effects[i].Turns {
				g.Effects[i].Turns = turns
			}
			return
		}
	}
	g.Effects = append(g.Effects, Effect{Kind: kind, Turns: turns})
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
			if g.PlayerHP <= 0 && !g.Dead {
				g.PlayerHP = 0
				g.Dead = true
				g.DeathCause = "poison"
				g.log("The poison overcomes you. You die.")
			}
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
