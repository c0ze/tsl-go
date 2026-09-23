package game

// Environmental hazards on the player's turn: traps and webs, lava, deep
// water, and the senses (blindness, gas masks, noticing) they interact with.

import (
	"github.com/c0ze/tsl-go/internal/content"
)

// passTurn is a single player turn. The per-turn bookkeeping (effect
// clocks, EP regen) runs once — in the C these happen per creature-turn, not
// per tick (game.c pass_time_on_effects/energy) — then the player pays for the
// action and the world ticks until the player has banked the next turn.
// playerEnergy holds the player's surplus beyond the turn just taken (the C's
// move_counter minus TURN_TIME), so at base speed exactly one tick passes; a
// slowed player owes two, and a hasted player sometimes none (a free action).
// playerPerception is the player's trap-spotting sense (C player.c:1265):
// only traps flimsier than it betray themselves on sight.
const playerPerception = 3

// isTrapTile reports whether stepping on def springs something: a status
// effect or straight damage (the electrified plate).
func isTrapTile(def *content.TileDef) bool {
	return def.Effect != "" || def.Damage != ""
}

// springTrapAt reveals the trap at p and inflicts it — the one trigger path
// shared by stepping, landing, and blinking (C activate_trap), dispatching
// per trap kind.
func (g *Game) springTrapAt(p Pos) {
	tile := g.Level.At(p)
	tile.Revealed = true
	def := tile.Def
	switch {
	case def.Damage != "": // the electrified plate (C PLATE_DAMAGE)
		g.log("You step on an electrified plate!")
		g.HurtPlayer(g.RNG.RollSpec(def.Damage), "electricity")
	case def.Effect == "polymorph": // the trap rolls the potion's dice
		g.log("You step on a polymorph trap!")
		g.log("%s", g.PolymorphRandom(def.EffectTurns))
	case def.Effect == "web":
		g.log("You get stuck in a web!")
		g.AddEffect("web", def.EffectTurns)
	case def.Effect == "blind":
		if !g.playerBlinded() { // a flash means nothing to covered (or blindfolded) eyes (C)
			g.log("You are blinded by a bright flash!")
			g.AddEffect("blind", def.EffectTurns)
		}
	case def.Effect != "":
		g.AddEffect(def.Effect, def.EffectTurns)
		g.log("You trigger a trap!")
	}
}

// webStruggle is the C's struggle_web: a move attempt while webbed tears
// WEB_STRUGGLE (6) off the clock instead of moving; under that, you're out.
const webStruggle = 6

func (g *Game) struggleWeb() {
	for i := range g.Effects {
		if g.Effects[i].Kind != "web" {
			continue
		}
		if g.Effects[i].Turns < webStruggle {
			g.RemoveEffect("web")
			g.log("You break free of the web.")
		} else {
			g.Effects[i].Turns -= webStruggle
			g.log("You struggle in the web!")
		}
		return
	}
}

// spotTraps is the C's try_to_detect_traps: each turn, a visible unrevealed
// trap is spotted when the player's perception beats its difficulty.
func (g *Game) spotTraps() {
	for i := range g.Level.tiles {
		t := &g.Level.tiles[i]
		if t.Disguise != nil && !t.Revealed && t.Visible && playerPerception > t.TrapDifficulty {
			t.Revealed = true
		}
	}
}

// lavaCheck burns a non-floating player standing in lava for 1d6+1 every
// turn (C elements.c lava_bath, LAVA_DAMAGE) — no fatigue grace, unlike water.
func (g *Game) lavaCheck() {
	if g.HasEffect("levitate") || !g.Level.At(g.Player).Def.Lava {
		return
	}
	g.log("You get burned by lava!")
	g.PlayerHP -= g.RNG.RollSpec("1d6+1")
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = "lava"
		g.log("You die...")
	}
}

// playerSwimming is the player's swimming skill — how many turns of deep
// water are free before fatigue bites (C attr_swimming, base 0 plus worn
// gear: flippers +3, lead boots -2).
func (g *Game) playerSwimming() int {
	skill := 0
	for _, it := range g.equippedGear() {
		skill += it.Def.SwimSkill
	}
	return skill
}

// playerBlinded reports whether the player cannot see — the blind effect or
// a worn blindfold (C attr_blindness as an item mod).
func (g *Game) playerBlinded() bool {
	if g.HasEffect("blind") {
		return true
	}
	return g.Head != nil && g.Head.Def != nil && g.Head.Def.Blindfold
}

// gasProtected reports whether worn gear seals the player's lungs
// (C attr_gas_immunity — the gas mask).
func (g *Game) gasProtected() bool {
	for _, it := range g.equippedGear() {
		if it.Def.GasImmune {
			return true
		}
	}
	return false
}

// noticeRange is how close a monster must be to notice the player: the
// global sense range shrunk by worn stealth (dark cloak, padded boots) —
// our mapping of the C's attr_stealth detection rolls. Floor 2: nothing
// misses you at arm's length.
func (g *Game) noticeRange() int {
	r := senseRange - g.playerStealth()
	if r < 2 {
		r = 2
	}
	return r
}

// swimCheck is the C's swim() for the player, run right after each turn's
// action: a turn spent in deep water adds fatigue, and past the swimming skill
// each turn costs 1 HP until the swimmer drowns; dry land resets the count.
func (g *Game) swimCheck() {
	if g.HasEffect("levitate") {
		return // airborne: the fatigue counter freezes (C swim() skips floaters)
	}
	if g.Shape != nil && g.Shape.Swim {
		return // a swimming form never drowns (C swim() free_swim)
	}
	if !g.Level.At(g.Player).Def.Water {
		g.swimFatigue = 0
		return
	}
	g.swimFatigue++
	if g.swimFatigue <= g.playerSwimming() {
		return
	}
	g.PlayerHP--
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = "drowning"
		g.log("You drown...")
	}
}
