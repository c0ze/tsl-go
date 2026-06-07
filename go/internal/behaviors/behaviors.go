// Package behaviors implements named item/ability effects as game.Behaviors and
// exposes them via Registry(). It imports game (to act on the world); game does
// not import it — cmd injects the registry into the Game, avoiding a cycle.
package behaviors

import (
	"fmt"

	"github.com/c0ze/tsl/internal/game"
)

// Registry returns the name→behavior map referenced by item `use` fields.
func Registry() map[string]game.Behavior {
	return map[string]game.Behavior{
		"heal": heal,
	}
}

func heal(g *game.Game, it *game.Item) []string {
	amt := it.Def.Power
	g.PlayerHP += amt
	if g.PlayerHP > g.PlayerMax {
		g.PlayerHP = g.PlayerMax
	}
	return []string{fmt.Sprintf("You quaff the %s and recover %d HP.", it.Def.Name, amt)}
}
