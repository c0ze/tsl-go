package game

import (
	"fmt"

	"github.com/c0ze/tsl-go/internal/content"
)

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
		g.playerAttacks(m) // the C checks enemies before the web: you can still fight
		acted = true
	} else if g.HasEffect("web") {
		g.struggleWeb() // a move attempt becomes the struggle (C struggle_web)
		acted = true
	} else if g.Move(d) {
		acted = true
		tile := g.Level.At(g.Player).Def
		if tile.Win {
			g.Won = true
			g.log("You ascend to demigodhood. You win!")
			return // winning ends the turn immediately
		}
		if isTrapTile(tile) && !g.HasEffect("levitate") {
			// A floater glides over floor traps (C activate_trap); Win above
			// already fired regardless, the C's trap_win exception.
			g.springTrapAt(g.Player)
		}
	} else if g.openDoor(dst) { // blocked by a closed door: open it (costs the turn)
		g.log("You open the door.")
		acted = true
	} else if g.Level.InBounds(dst) && (g.Level.At(dst).Def.Water || g.Level.At(dst).Def.Lava) &&
		(g.HasEffect("blind") || g.HasEffect("levitate") ||
			(g.Level.At(dst).Def.Water && g.Shape != nil && g.Shape.Swim)) {
		// Deep water and lava turn away a sighted walker; the blinded blunder
		// in and floaters drift across (C move_creature).
		g.Player = dst
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
	if g.Shape != nil {
		return g.Shape.Attack // the form's claws; gear grants nothing (C shapeshift)
	}
	atk := playerAttack
	for _, it := range g.equippedGear() {
		atk += it.Def.Attack
	}
	return atk
}

// flameHandsDamage approximates the C's virtual_flame_hands attack sequence
// (vweapon.c: a deterministic 3/5/5/5 fire chain — avg 4.5, max 5) in our
// flat-dice model: 1d2+3 matches both the average and the cap (min 4 vs the
// C's occasional 3).
const flameHandsDamage = "1d2+3"

func (g *Game) playerDamageSpec() string {
	if g.HasEffect("hungry_book") {
		// The cursed temp weapon also supersedes the wielded one.
		return hungryBookDamage
	}
	if g.HasEffect("flame_hands") {
		// A temp weapon supersedes the wielded one (C set_temp_weapon).
		return flameHandsDamage
	}
	if g.Shape != nil && g.Shape.Damage != "" {
		return g.Shape.Damage // the form's natural attack
	}
	if g.Weapon != nil && g.Weapon.Def.Damage != "" {
		return g.Weapon.Def.Damage
	}
	return playerDamage
}

func (g *Game) playerDodgeStat() int {
	if g.Shape != nil {
		return g.Shape.Dodge
	}
	dodge := playerDodge
	for _, it := range g.equippedGear() {
		dodge += it.Def.Dodge
	}
	return dodge
}

// revealMimic tears the glamour off a disguised mimic (C reveal_mimic). It
// reports whether a reveal happened — the swing that finds out is wasted.
func (g *Game) revealMimic(m *Creature) bool {
	if !m.Disguised {
		return false
	}
	m.Disguised = false
	g.log("Wait! That is a small mimic!")
	return true
}

func (g *Game) playerAttacks(m *Creature) {
	if g.revealMimic(m) {
		return // turn wasted! (C combat.c:237)
	}
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
		g.revealMimic(m) // a bolt tears the glamour (C reveal_mimic)
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
	if !g.Dead && m.Def.Effect != "" { // venomous bites etc. (C virtual weapons)
		g.AddEffect(m.Def.Effect, m.Def.EffectTurns)
		g.log("The %s %s you.", m.Def.Name, effectVerb(m.Def.Effect))
	}
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
		if !g.HasEffect("blind") { // a flash means nothing to covered eyes (C)
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

// playerSwimming is the player's swimming skill — how many turns of deep water
// are free before fatigue bites (C attr_swimming, base 0; gear may raise it
// when the attribute lands).
const playerSwimming = 0

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
	if g.swimFatigue <= playerSwimming {
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
		if m.Lifetime > 0 {
			m.Lifetime--
			if m.Lifetime == 0 {
				// A summon's time is up: it vanishes without corpse or drops
				// (C game.c lifetime handling).
				g.log("The %s disappears.", m.Def.Name)
				g.Level.RemoveCreature(m)
				continue
			}
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
	if m.Disguised {
		return // a mimic in its glamour does nothing at all (C ai_mimic)
	}
	if m.HasEffect("web") {
		// The monster's turn is its struggle (C struggle_web).
		for i := range m.Effects {
			if m.Effects[i].Kind == "web" {
				if m.Effects[i].Turns < webStruggle {
					m.RemoveEffect("web")
				} else {
					m.Effects[i].Turns -= webStruggle
				}
				break
			}
		}
		return
	}
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
	if m.Ally {
		g.allyAct(m)
		return
	}
	dist := chebyshev(m.Pos, g.Player)
	if dist == 1 {
		g.monsterAttacks(m)
		return
	}
	// A distant player doesn't shield an ally at the flank (C enemies split).
	for _, c := range g.Level.Creatures {
		if c.Ally && chebyshev(m.Pos, c.Pos) == 1 {
			g.monsterFights(m, c)
			return
		}
	}
	if m.HasEffect("blind") {
		return // blinded: it can flail at an adjacent foe but can't track at range
	}
	if r := breathRange(m); r > 0 && dist <= r && g.lineOfSight(m.Pos, g.Player) {
		g.breathe(m)
		return
	}
	if m.Def.Ranged > 0 && dist <= m.Def.Ranged && g.lineOfSight(m.Pos, g.Player) {
		g.rangedAttack(m)
		return
	}
	if dist <= senseRange {
		g.stepToward(m, g.Player)
	}
}

// allyAct is a charmed creature's turn (C charm/ai_offensive): bite the
// adjacent hostile, else close on the nearest one in sense range, else heel
// to the player.
func (g *Game) allyAct(m *Creature) {
	var target *Creature
	best := senseRange + 1
	for _, c := range g.Level.Creatures {
		if c == m || c.Ally {
			continue
		}
		if d := chebyshev(m.Pos, c.Pos); d < best {
			best, target = d, c
		}
	}
	if target != nil && best == 1 {
		g.monsterFights(m, target)
		return
	}
	if target != nil {
		g.stepToward(m, target.Pos)
		return
	}
	if chebyshev(m.Pos, g.Player) > 2 {
		g.stepToward(m, g.Player)
	}
}

// monsterFights resolves one creature-vs-creature swing — an ally biting a
// hostile or the reverse — on the same attack-vs-dodge model as every other
// melee in the port.
func (g *Game) monsterFights(a, d *Creature) {
	g.revealMimic(d)
	if !g.RNG.Chance(a.Def.Attack, d.Def.Dodge) {
		g.log("The %s misses the %s.", a.Def.Name, d.Def.Name)
		return
	}
	d.RemoveEffect("sleep") // a landed hit wakes a sleeper (C combat.c)
	dmg := g.RNG.RollSpec(a.Def.Damage)
	d.HP -= dmg
	g.log("The %s hits the %s for %d.", a.Def.Name, d.Def.Name, dmg)
	if d.HP <= 0 {
		g.killCreature(d)
	}
}

// creatureCanEnter is the terrain half of a monster's move (C move_creature):
// a permaswimmer refuses anything but water, a free-swimmer takes water or
// land, and everyone else needs walkable ground.
func (g *Game) creatureCanEnter(m *Creature, dst Pos) bool {
	if !g.Level.InBounds(dst) {
		return false
	}
	if m.Def.Mimic {
		return false // rooted in place (C attr_p_move)
	}
	if m.Def.Permaswim {
		return g.Level.At(dst).Def.Water
	}
	if m.Def.Swim && g.Level.At(dst).Def.Water {
		return true
	}
	return g.Level.Passable(dst)
}

func (g *Game) stepToward(m *Creature, target Pos) {
	dst := Pos{m.Pos.X + signOf(target.X-m.Pos.X), m.Pos.Y + signOf(target.Y-m.Pos.Y)}
	if dst == g.Player || g.Level.CreatureAt(dst) != nil {
		return
	}
	if !g.creatureCanEnter(m, dst) {
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
	if dst == g.Player || g.Level.CreatureAt(dst) != nil || !g.creatureCanEnter(m, dst) {
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
	if dst == g.Player || g.Level.CreatureAt(dst) != nil || !g.creatureCanEnter(m, dst) {
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
