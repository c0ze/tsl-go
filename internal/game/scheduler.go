package game

// The turn scheduler (C game.c): speeds, burden, and the energy loop that
// lets every creature act at its own pace between the player's turns.

const (
	turnCost     = 100 // energy needed for one action
	defaultSpeed = 100 // energy per tick for an average creature (C rules.h BASE_SPEED)
	hasteBonus   = 30  // flat speed bonus while hasted (C rules.h HASTE_AMOUNT)
)

// speedOf is a monster's energy gain per tick (a non-positive def speed means
// "average").
func speedOf(m *Creature) int {
	if m.Def.Speed > 0 {
		return m.Def.Speed
	}
	return defaultSpeed
}

// carryCapacity is how much the player can haul comfortably
// (C rules.h DEFAULT_CARRYING_CAPACITY).
const carryCapacity = 400

// carriedWeight sums the pack, equipped gear included — worn items stay in
// the inventory list, as in the C.
func (g *Game) carriedWeight() int {
	total := 0
	for _, it := range g.Inventory {
		if it == nil || it.Def == nil {
			continue
		}
		if it.Def.Kind == "ammo" {
			total += it.Charges * it.Def.Weight // a stack weighs per arrow (C burdened.c)
			continue
		}
		total += it.Def.Weight
	}
	return total
}

// burdened reports whether the player carries more than the allowance
// (C burdened.c is_burdened).
func (g *Game) burdened() bool { return g.carriedWeight() > carryCapacity }

// playerSpeed is the player's energy gain per world tick (the C's attr_speed,
// base BASE_SPEED): haste adds a flat bonus, slow halves — the same rule
// monsters use (the C's literal -1 for slow is vestigial on a 100 scale) —
// and a heavy pack halves what's left (C BURDENED_FACTOR, applied after the
// effect mods as in increment_counters).
func (g *Game) playerSpeed() int {
	speed := defaultSpeed
	if g.Shape != nil && g.Shape.Speed > 0 {
		speed = g.Shape.Speed // the form's pace, modifiers on top
	}
	for _, it := range g.equippedGear() {
		speed += it.Def.SpeedMod // boots of speed, the seaweed cloak, lead boots
	}
	if g.HasEffect("haste") {
		speed += hasteBonus
	}
	if g.HasEffect("slow") {
		speed /= 2
	}
	if g.burdened() {
		speed /= 2
	}
	if speed < 1 {
		speed = 1
	}
	return speed
}

// advanceWorld passes the player's turn — and, while the player is asleep,
// keeps passing turns with no player action, the way the C main loop skips a
// sleeping creature's turns without prompting (game.c effect_sleep). Sleep
// ends by tickEffects expiry or by being hit awake (HurtPlayer), so the loop
// is bounded by the effect's duration.
func (g *Game) advanceWorld() {
	g.passTurn()
	for g.HasEffect("sleep") && !g.Dead {
		g.passTurn()
	}
}

func (g *Game) passTurn() {
	g.swimCheck()
	if g.Dead {
		return // drowned
	}
	g.lavaCheck()
	if g.Dead {
		return // melted
	}
	g.spotTraps()
	g.tickEffects()
	if g.Dead {
		return // status effects (e.g. poison) killed the player
	}
	g.regenEP()
	g.playerEnergy -= turnCost
	for g.playerEnergy < 0 {
		g.playerEnergy += g.playerSpeed()
		g.worldTick()
		if g.Dead {
			return
		}
	}
}

// worldTick is one tick of world time. Each living monster gains energy equal
// to its speed and acts for every full turn's worth it holds, so faster
// monsters act more often (the C's move_counter / TURN_TIME model). Leftover
// energy carries to the next tick.
func (g *Game) worldTick() {
	// snapshot to avoid mutation surprises when creatures are removed
	snapshot := make([]*Creature, len(g.Level.Creatures))
	copy(snapshot, g.Level.Creatures)
	for _, m := range snapshot {
		if g.Dead {
			return
		}
		if g.Level.CreatureAt(m.Pos) != m {
			continue // already removed this turn
		}
		gain := speedOf(m)
		if m.HasEffect("slow") {
			gain /= 2 // slowed creatures bank energy at half rate
		}
		m.Energy += gain
		for m.Energy >= turnCost {
			// A creature's clocks run on its own turns, not the world's
			// ticks (C game.c: lifetime and pass_time_on_effects only once
			// move_counter reaches TURN_TIME) — a slow zombie stays asleep
			// as many of its turns as a quick rat.
			m.Energy -= turnCost
			if m.Lifetime > 0 {
				m.Lifetime--
				if m.Lifetime == 0 {
					// A summon's time is up: it vanishes without corpse or
					// drops (C game.c lifetime handling).
					g.log("The %s disappears.", m.Def.Name)
					g.Level.RemoveCreature(m)
					break
				}
			}
			if g.tickCreatureEffects(m) {
				break // succumbed to its afflictions before acting
			}
			g.monsterAct(m)
			if g.Dead {
				return
			}
			if g.Level.CreatureAt(m.Pos) != m {
				break // removed mid-turn
			}
		}
	}
}
