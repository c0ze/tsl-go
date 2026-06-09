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
		"heal":            heal,
		"eat":             eat,
		"regenerate":      regenerate,
		"eat_mushroom":    eatMushroom,
		"teleport":        teleport,
		"reveal":          reveal,
		"instant_healing": instantHealing,
		"pain":            pain,
		"identify":        identifyScroll,
		"recharge":        recharge,
		"first_aid":       firstAid,
		"blindness":       blindness,
		"flash":           flash,
		"restore_energy":  restoreEnergy,
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

// restoreEP adds amount to the player's EP, clamped to EPMax, and reports how
// much was actually recovered.
func restoreEP(g *game.Game, amount int) int {
	before := g.EP
	g.EP += amount
	if g.EP > g.EPMax {
		g.EP = g.EPMax
	}
	return g.EP - before
}

// restoreEnergy refills spell energy (a potion of energy).
func restoreEnergy(g *game.Game, it *game.Item) []string {
	gained := restoreEP(g, it.Def.Power)
	return []string{fmt.Sprintf("You quaff the %s and your mind sharpens (+%d EP).", it.Def.Name, gained)}
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

// instantHealing restores a big chunk of HP at once (the faithful 10 + rnd%15).
func instantHealing(g *game.Game, it *game.Item) []string {
	recovered := restoreHP(g, 10+g.RNG.Intn(15))
	return []string{fmt.Sprintf("You quaff the %s and surge with vitality (+%d HP).", it.Def.Name, recovered)}
}

// pain damages the drinker — the reason you don't quaff unknown potions blindly.
func pain(g *game.Game, it *game.Item) []string {
	g.HurtPlayer(it.Def.Power, "a potion of pain")
	return []string{fmt.Sprintf("The %s wracks you with agony!", it.Def.Name)}
}

// identifyScroll reveals one still-unidentified item in the pack.
func identifyScroll(g *game.Game, it *game.Item) []string {
	unknown := g.UnidentifiedInventory()
	if len(unknown) == 0 {
		return []string{"You sense nothing new about your possessions."}
	}
	pick := unknown[g.RNG.Intn(len(unknown))]
	was := g.DisplayName(pick)
	g.IdentifyItem(pick)
	return []string{fmt.Sprintf("Your %s is revealed to be a %s.", was, pick.Def.Name)}
}

// blindness blinds the drinker for Power turns — a faithful "bad" potion.
func blindness(g *game.Game, it *game.Item) []string {
	g.AddEffect("blind", it.Def.Power)
	return []string{fmt.Sprintf("You drink the %s and the world goes dark!", it.Def.Name)}
}

// flash is the flash spell — a burst of light that blinds the creatures around
// the caster, breaking the pack's pursuit.
func flash(g *game.Game, it *game.Item) []string {
	n := g.FlashBlind(game.FlashRadius, it.Def.Power)
	if n == 0 {
		return []string{fmt.Sprintf("You cast %s, but no creatures are near.", it.Def.Name)}
	}
	return []string{fmt.Sprintf("Light erupts from %s, blinding %d nearby creature(s)!", it.Def.Name, n)}
}

// firstAid is the first-aid spell — it knits wounds over time (a regen effect),
// the faithful "applies a healing effect" from magic.c.
func firstAid(g *game.Game, it *game.Item) []string {
	g.AddEffect("regen", it.Def.Power)
	return []string{fmt.Sprintf("You weave %s; your wounds begin to knit shut.", it.Def.Name)}
}

// recharge tops up a random carried wand with a few fresh charges.
func recharge(g *game.Game, it *game.Item) []string {
	wands := g.WandInventory()
	if len(wands) == 0 {
		return []string{"You feel a surge of magic with nowhere to go."}
	}
	w := wands[g.RNG.Intn(len(wands))]
	add := 3 + g.RNG.Intn(3)
	w.Charges += add
	return []string{fmt.Sprintf("Your %s crackles with %d fresh charges.", g.DisplayName(w), add)}
}
