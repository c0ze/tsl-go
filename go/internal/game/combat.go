package game

import "fmt"

// Player melee stats (constant until the player becomes attribute-driven).
const (
	playerAttack = 5
	playerDodge  = 3
	playerDamage = "1d4"
)

const senseRange = 8 // how close a monster must be to notice the player

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

// playerSpeed is the player's energy gain per world tick (the C's attr_speed,
// base BASE_SPEED): haste adds a flat bonus, slow halves — the same rule
// monsters use (the C's literal -1 for slow is vestigial on a 100 scale).
func (g *Game) playerSpeed() int {
	speed := defaultSpeed
	if g.HasEffect("haste") {
		speed += hasteBonus
	}
	if g.HasEffect("slow") {
		speed /= 2
	}
	if speed < 1 {
		speed = 1
	}
	return speed
}

// log appends a message to the game log.
func (g *Game) log(format string, args ...any) {
	g.Messages = append(g.Messages, fmt.Sprintf(format, args...))
}

// PlayerStep performs the player's turn: bump-attack a monster in direction d,
// otherwise move. Then every monster acts.
func (g *Game) PlayerStep(d Direction) {
	if g.Dead || g.Won {
		return
	}
	dx, dy := d.Delta()
	dst := Pos{g.Player.X + dx, g.Player.Y + dy}
	acted := false
	if m := g.Level.CreatureAt(dst); m != nil {
		g.playerAttacks(m)
		acted = true
	} else if g.Move(d) {
		acted = true
		tile := g.Level.At(g.Player).Def
		if tile.Win {
			g.Won = true
			g.log("You ascend to demigodhood. You win!")
			return // winning ends the turn immediately
		}
		if tile.Effect != "" {
			g.AddEffect(tile.Effect, tile.EffectTurns)
			g.log("You trigger a trap!")
		}
	} else if g.openDoor(dst) { // blocked by a closed door: open it (costs the turn)
		g.log("You open the door.")
		acted = true
	}
	if acted { // a blocked move into a wall doesn't pass the turn
		g.advanceWorld()
	}
}

// equippedGear returns the items the player has worn or wielded, in slot order.
// Accessories (ring, amulet) sit alongside weapon and armor so their bonuses
// stack into the combat stats below.
func (g *Game) equippedGear() []*Item {
	var worn []*Item
	for _, it := range []*Item{g.Weapon, g.Armor, g.Ring, g.Amulet} {
		if it != nil && it.Def != nil {
			worn = append(worn, it)
		}
	}
	return worn
}

func (g *Game) playerAttackStat() int {
	atk := playerAttack
	for _, it := range g.equippedGear() {
		atk += it.Def.Attack
	}
	return atk
}

func (g *Game) playerDamageSpec() string {
	if g.Weapon != nil && g.Weapon.Def.Damage != "" {
		return g.Weapon.Def.Damage
	}
	return playerDamage
}

func (g *Game) playerDodgeStat() int {
	dodge := playerDodge
	for _, it := range g.equippedGear() {
		dodge += it.Def.Dodge
	}
	return dodge
}

func (g *Game) playerAttacks(m *Creature) {
	if !g.RNG.Chance(g.playerAttackStat(), m.Def.Dodge) {
		g.log("You miss the %s.", m.Def.Name)
		return
	}
	m.RemoveEffect("sleep") // a landed hit wakes a sleeper; a whiff doesn't (C combat.c)
	dmg := g.RNG.RollSpec(g.playerDamageSpec())
	m.HP -= dmg
	g.log("You hit the %s for %d.", m.Def.Name, dmg)
	if m.HP <= 0 {
		g.killCreature(m)
	}
}

// killCreature resolves a monster's death: announce it, drop its corpse (when
// the def names one), and remove it from the level. Centralising death here
// keeps every kill site (player now, hazards later) dropping corpses consistently.
func (g *Game) killCreature(m *Creature) {
	g.log("The %s dies.", m.Def.Name)
	if m.Def.Corpse != "" && g.Content != nil {
		if def, ok := g.Content.Items[m.Def.Corpse]; ok {
			g.Level.Items = append(g.Level.Items, &Item{Def: def, Pos: m.Pos})
		}
	}
	g.Level.RemoveCreature(m)
}

// ZapWand fires a wand at target: it spends a charge, then on the creature there
// deals the wand's damage (if any, killing it at 0 HP) and — if the target
// survives — applies the wand's status effect (e.g. poison). A wand may carry
// damage, an effect, or both. It passes a turn; a wand with no charges fizzles
// without costing a turn.
func (g *Game) ZapWand(it *Item, target Pos) {
	if g.Dead || g.Won {
		return
	}
	if it.Charges <= 0 {
		g.log("The %s has no charges left.", it.Def.Name)
		return
	}
	it.Charges--
	g.identify(it) // zapping a wand reveals its type
	if m := g.Level.CreatureAt(target); m != nil {
		if it.Def.Damage != "" {
			dmg := g.RNG.RollSpec(it.Def.Damage)
			m.HP -= dmg
			g.log("The %s blasts the %s for %d.", it.Def.Name, m.Def.Name, dmg)
		}
		if m.HP <= 0 {
			g.killCreature(m)
		} else if it.Def.Effect != "" {
			m.AddEffect(it.Def.Effect, it.Def.EffectTurns)
			g.log("The %s %s the %s.", it.Def.Name, effectVerb(it.Def.Effect), m.Def.Name)
		}
	} else {
		g.log("The bolt fizzles against nothing.")
	}
	g.advanceWorld()
}

func (g *Game) monsterAttacks(m *Creature) {
	if !g.RNG.Chance(m.Def.Attack, g.playerDodgeStat()) {
		g.log("The %s misses you.", m.Def.Name)
		return
	}
	dmg := g.RNG.RollSpec(m.Def.Damage)
	g.log("The %s hits you for %d.", m.Def.Name, dmg)
	g.HurtPlayer(dmg, m.Def.Name)
}

// rangedAttack fires a bolt at the player from a distance, using the same
// to-hit (attack vs dodge) and damage model as a melee swing.
func (g *Game) rangedAttack(m *Creature) {
	if !g.RNG.Chance(m.Def.Attack, g.playerDodgeStat()) {
		g.log("The %s's bolt misses you.", m.Def.Name)
		return
	}
	dmg := g.RNG.RollSpec(m.Def.Damage)
	g.log("The %s blasts you for %d.", m.Def.Name, dmg)
	g.HurtPlayer(dmg, m.Def.Name)
}

// HurtPlayer applies dmg to the player and resolves death once (clamped HP +
// recorded cause), so melee and ranged attacks share one death path.
func (g *Game) HurtPlayer(dmg int, cause string) {
	g.PlayerHP -= dmg
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = cause
		g.log("You die.")
		return
	}
	if g.HasEffect("sleep") { // pain cuts sleep short (C combat.c wakes the defender)
		g.RemoveEffect("sleep")
		g.log("You wake up!")
	}
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

// passTurn is a single player turn. The per-turn bookkeeping (effect
// clocks, EP regen) runs once — in the C these happen per creature-turn, not
// per tick (game.c pass_time_on_effects/energy) — then the player pays for the
// action and the world ticks until the player has banked the next turn.
// playerEnergy holds the player's surplus beyond the turn just taken (the C's
// move_counter minus TURN_TIME), so at base speed exactly one tick passes; a
// slowed player owes two, and a hasted player sometimes none (a free action).
func (g *Game) passTurn() {
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
		// Capture slow before ticking, so a 1-turn slow still applies this turn
		// (symmetric with poison dealing its final tick before expiring).
		slowed := m.HasEffect("slow")
		if g.tickCreatureEffects(m) {
			continue // succumbed to its afflictions before acting
		}
		gain := speedOf(m)
		if slowed {
			gain /= 2 // slowed creatures bank energy at half rate
		}
		m.Energy += gain
		for m.Energy >= turnCost {
			m.Energy -= turnCost
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

// monsterAct is a single monster action: attack the player if adjacent, else
// step toward the player if within sense range.
func (g *Game) monsterAct(m *Creature) {
	if m.HasEffect("sleep") {
		return // asleep: it loses the turn outright (C game.c effect_sleep skip)
	}
	if m.HasEffect("confuse") {
		g.stepRandom(m) // disoriented: it lurches at random and can't press an attack
		return
	}
	if m.HasEffect("fear") {
		g.stepAway(m, g.Player) // frightened: it flees the player instead of attacking
		return
	}
	dist := chebyshev(m.Pos, g.Player)
	if dist == 1 {
		g.monsterAttacks(m)
		return
	}
	if m.HasEffect("blind") {
		return // blinded: it can flail at an adjacent foe but can't track at range
	}
	if m.Def.Ranged > 0 && dist <= m.Def.Ranged && g.lineOfSight(m.Pos, g.Player) {
		g.rangedAttack(m)
		return
	}
	if dist <= senseRange {
		g.stepToward(m, g.Player)
	}
}

func (g *Game) stepToward(m *Creature, target Pos) {
	dst := Pos{m.Pos.X + signOf(target.X-m.Pos.X), m.Pos.Y + signOf(target.Y-m.Pos.Y)}
	if dst == g.Player || g.Level.CreatureAt(dst) != nil {
		return
	}
	if !g.Level.Passable(dst) {
		g.openDoor(dst) // open a door in the way (spends this move); a plain wall is a no-op
		return
	}
	m.Pos = dst
}

// stepAway moves m one tile directly away from `from` — a frightened creature's
// flight. If the retreat tile is blocked (a wall, another creature, or the
// player), the cornered creature holds its ground.
func (g *Game) stepAway(m *Creature, from Pos) {
	dx, dy := signOf(m.Pos.X-from.X), signOf(m.Pos.Y-from.Y)
	if dx == 0 && dy == 0 {
		return // on top of the player (shouldn't happen): nowhere to flee
	}
	dst := Pos{m.Pos.X + dx, m.Pos.Y + dy}
	if dst == g.Player || g.Level.CreatureAt(dst) != nil || !g.Level.Passable(dst) {
		return
	}
	m.Pos = dst
}

// stepRandom moves m one tile in a random direction — a confused creature's
// aimless lurch. It won't stumble onto the player (so a confused creature never
// attacks), onto another creature, or into a wall; those just cost the turn.
func (g *Game) stepRandom(m *Creature) {
	dx, dy := g.RNG.Intn(3)-1, g.RNG.Intn(3)-1
	if dx == 0 && dy == 0 {
		return // stumbles in place
	}
	dst := Pos{m.Pos.X + dx, m.Pos.Y + dy}
	if dst == g.Player || g.Level.CreatureAt(dst) != nil || !g.Level.Passable(dst) {
		return
	}
	m.Pos = dst
}

func chebyshev(a, b Pos) int {
	dx, dy := a.X-b.X, a.Y-b.Y
	if dx < 0 {
		dx = -dx
	}
	if dy < 0 {
		dy = -dy
	}
	if dx > dy {
		return dx
	}
	return dy
}
