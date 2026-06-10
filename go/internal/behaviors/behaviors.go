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
		"noxious_cloud":   noxiousCloud,
		"scare":           scare,
		"restore_energy":  restoreEnergy,
		"haste":           haste,
		"slowness":        slowness,
		"tranquilize":     tranquilize,
		"levitate":        levitate,
		"elixir":          elixir,
		"yuck":            yuck,
		"amnesia":         amnesia,
		"mark":            mark,
		"recall":          recall,
		"detect_traps":    detectTraps,
		"magic_weapon":    magicWeapon,
		"summon_familiar": summonFamiliar,
		"pharmacy":        pharmacy,
	}
}

// pharmacy reveals the whole potion table at once (C reading.c: the manual
// of pharmacy).
func pharmacy(g *game.Game, it *game.Item) []string {
	g.IdentifyAllPotions()
	return []string{"You learn how to identify all potions."}
}

// summonFamiliar calls forth an imp ally for Power ticks (C magic.c
// summon_familiar: always an imp — the 1d4 switch has only a default —
// charmed, DEFAULT_SUMMON_LIFETIME).
func summonFamiliar(g *game.Game, it *game.Item) []string {
	if g.SummonAlly("imp", it.Def.Power) {
		return []string{"The imp has arrived."}
	}
	return []string{fmt.Sprintf("You read the %s, but nothing happens.", it.Def.Name)}
}

// magicWeapon ignites the reader's hands for Power turns, superseding any
// wielded weapon (C magic.c magic_weapon / set_temp_weapon).
func magicWeapon(g *game.Game, it *game.Item) []string {
	g.AddEffect("flame_hands", it.Def.Power)
	return []string{"Your hands seem to be on fire."}
}

// detectTraps lights up every hidden trap on the level (C magic.c
// reveal_traps; the C queues its message whether or not anything was found).
func detectTraps(g *game.Game, it *game.Item) []string {
	g.RevealTraps()
	return []string{"You sense the presence of traps."}
}

// mark pins the spot a recall scroll will return to (C teleport.c cast_mark).
func mark(g *game.Game, it *game.Item) []string {
	g.MarkRecall()
	return []string{"Destination marked."}
}

// recall snaps the reader back to the mark, across levels (C cast_recall).
func recall(g *game.Game, it *game.Item) []string {
	if g.Recall() {
		return []string{fmt.Sprintf("You find yourself back in %s.", recallPlace(g))}
	}
	return []string{fmt.Sprintf("You read the %s, but nothing happens.", it.Def.Name)}
}

// recallPlace names the arrival level, with a fallback for bare unit-test
// games that have no dungeon wired.
func recallPlace(g *game.Game) string {
	if name := g.LocationName(); name != "" {
		return name
	}
	return "familiar surroundings"
}

// amnesia wipes the automap (C magic.c amnesia) — the priciest thing an
// unidentified scroll can cost you is what you already knew.
func amnesia(g *game.Game, it *game.Item) []string {
	g.ForgetMap()
	return []string{"You suddenly feel very forgetful."}
}

// elixir strips every status the C expires — poison, haste, slow, regen,
// blindness — and cuts levitation short, landing the drinker (C potions.c
// treasure_elixir: "Removes all status effects, even good ones").
func elixir(g *game.Game, it *game.Item) []string {
	for _, kind := range []string{"poison", "haste", "slow", "regen", "blind"} {
		g.RemoveEffect(kind)
	}
	g.DispelLevitation()
	return []string{"You feel perfectly normal."}
}

// yuck is pure flavor — a vile taste and nothing else (C treasure_p_yuck's
// 1d4 roll: gnoblin blood on 2-3).
func yuck(g *game.Game, it *game.Item) []string {
	switch g.RNG.Intn(4) {
	case 0:
		return []string{"This tastes like ratman urine."}
	case 3:
		return []string{"This tastes like liquified maggots."}
	default:
		return []string{"This tastes like gnoblin blood."}
	}
}

// levitate lifts the player off the ground for Power turns (C potions.c
// treasure_p_levitation → altitude.c): water and floor traps pass harmlessly
// below until the landing.
func levitate(g *game.Game, it *game.Item) []string {
	g.AddEffect("levitate", it.Def.Power)
	return []string{"You soar into the air!"}
}

// tranquilize knocks the player out for Power turns (C sleep.c tranquilize via
// potions.c treasure_p_sleep); the slept turns play out in advanceWorld.
func tranquilize(g *game.Game, it *game.Item) []string {
	g.AddEffect("sleep", it.Def.Power)
	return []string{"You fall asleep!"}
}

// haste speeds the player up for Power turns, cancelling an active slow
// (C potions.c treasure_p_speed).
func haste(g *game.Game, it *game.Item) []string {
	g.RemoveEffect("slow")
	g.AddEffect("haste", it.Def.Power)
	return []string{"You move faster!"}
}

// slowness drags the player down for Power turns, cancelling an active haste
// (C potions.c treasure_p_slowing).
func slowness(g *game.Game, it *game.Item) []string {
	g.RemoveEffect("haste")
	g.AddEffect("slow", it.Def.Power)
	return []string{"You feel very sluggish."}
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
		// The C's blink lines (teleport.c cast_blink).
		return []string{"You blink away...", "Suddenly, you are somewhere else."}
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

// noxiousCloud is the noxious cloud spell — a billow of poison gas that afflicts
// the creatures around the caster with damage-over-time (poison drains their HP
// each turn until it fades or kills).
func noxiousCloud(g *game.Game, it *game.Item) []string {
	n := g.PoisonNearby(game.NoxiousRadius, it.Def.Power)
	if n == 0 {
		return []string{fmt.Sprintf("You cast %s, but no creatures are near.", it.Def.Name)}
	}
	return []string{fmt.Sprintf("A cloud of poison billows from %s, choking %d nearby creature(s)!", it.Def.Name, n)}
}

// scare is the scroll of scare monster — a wave of terror that sends the nearby
// creatures fleeing instead of pressing their attack.
func scare(g *game.Game, it *game.Item) []string {
	n := g.ScareNearby(game.ScareRadius, it.Def.Power)
	if n == 0 {
		return []string{fmt.Sprintf("You read the %s, but no creatures are near.", it.Def.Name)}
	}
	return []string{fmt.Sprintf("You read the %s; %d nearby creature(s) turn and flee!", it.Def.Name, n)}
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
