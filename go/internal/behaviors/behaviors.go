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
		"heal":         heal,
		"eat":          eat,
		"regenerate":   regenerate,
		"eat_mushroom": eatMushroom,
		"teleport":     teleport,
		"reveal":       reveal,
	}
}

// restoreHP adds amount to the player's HP, clamped to PlayerMax, and reports
// how much was actually recovered.
func restoreHP(g *game.Game, amount int) int {
	before := g.PlayerHP
	g.PlayerHP += amount
	if g.PlayerHP > g.PlayerMax {
		g.PlayerHP = g.PlayerMax
	}
	return g.PlayerHP - before
}

func heal(g *game.Game, it *game.Item) []string {
	recovered := restoreHP(g, it.Def.Power)
	return []string{fmt.Sprintf("You quaff the %s and recover %d HP.", it.Def.Name, recovered)}
}

func eat(g *game.Game, it *game.Item) []string {
	recovered := restoreHP(g, it.Def.Power)
	if recovered > 0 {
		return []string{fmt.Sprintf("You eat the %s and recover %d HP.", it.Def.Name, recovered)}
	}
	return []string{fmt.Sprintf("You eat the %s.", it.Def.Name)}
}

// regenerate grants healing-over-time for Power turns.
func regenerate(g *game.Game, it *game.Item) []string {
	g.AddEffect("regen", it.Def.Power)
	return []string{fmt.Sprintf("You quaff the %s; your wounds begin to close.", it.Def.Name)}
}

// eatMushroom heals by Power but poisons the eater (the faithful red mushroom).
func eatMushroom(g *game.Game, it *game.Item) []string {
	recovered := restoreHP(g, it.Def.Power)
	g.AddEffect("poison", 6)
	return []string{fmt.Sprintf("You eat the %s and recover %d HP, but you feel ill.", it.Def.Name, recovered)}
}

// teleport blinks the player to a random spot on the level (an escape hatch).
func teleport(g *game.Game, it *game.Item) []string {
	if g.Teleport() {
		return []string{fmt.Sprintf("You read the %s and blink across the dungeon.", it.Def.Name)}
	}
	return []string{fmt.Sprintf("You read the %s, but nothing happens.", it.Def.Name)}
}

// reveal maps the whole level (a scroll of magic mapping).
func reveal(g *game.Game, it *game.Item) []string {
	g.RevealMap()
	return []string{fmt.Sprintf("You read the %s; the layout floods into your mind.", it.Def.Name)}
}
