package game

import (
	"fmt"
)

// Player melee stats (constant until the player becomes attribute-driven).
const (
	playerAttack = 5
	playerDodge  = 3
	playerDamage = "1d4"
)

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
	from := g.Player
	acted := false
	m := g.Level.CreatureAt(dst)
	if m != nil && !m.Ally {
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
		g.Sound("step")
	} else if g.revealSecretDoor(dst) { // the discovering bump costs the turn
		acted = true
	} else if g.bumpLockedDoor(dst) {
		// The unlock/force prompts run in the front-end; no turn passes here.
	} else if g.Shape != nil && g.Shape.NoDoors && g.Level.InBounds(dst) && g.Level.At(dst).Def.OpensTo != "" {
		g.log("As a %s, you cannot open doors.", g.Shape.Name) // free, as in the C
	} else if g.openDoor(dst) { // blocked by a closed door: open it (costs the turn)
		g.log("You open the door.")
		acted = true
	} else if g.Level.InBounds(dst) && g.Level.At(dst).Def.Water {
		// The player wades straight into deep water; the swim clock takes it
		// from there (C try_to_move, player.c:1514).
		g.Player = dst
		g.Sound("splash")
		acted = true
	} else if g.Level.InBounds(dst) && g.Level.At(dst).Def.Lava {
		acted = g.stepIntoLava(dst)
	}
	if m != nil && m.Ally && g.Player == dst {
		m.Pos = from // walking into an ally swaps places with it (C move_creature displace)
	}
	if g.Player != from && !g.Won {
		g.playerBleeds() // an open wound bleeds with every step (C wound_damage)
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
	for _, it := range []*Item{g.Weapon, g.Armor, g.Ring, g.Amulet, g.Boots, g.Head, g.Cloak} {
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
	g.Sound("swoosh") // the swing, whether or not it lands
	if !g.RNG.Chance(g.playerAttackStat(), m.Def.Dodge) {
		g.log("You miss the %s.", m.Def.Name)
		return
	}
	m.RemoveEffect("sleep") // a landed hit wakes a sleeper; a whiff doesn't (C combat.c)
	dmg := g.RNG.RollSpec(g.playerDamageSpec())
	m.HP -= dmg
	g.log("You hit the %s for %d.", m.Def.Name, dmg)
	g.Sound("hit")
	if m.HP <= 0 {
		g.killCreature(m)
	} else if g.rollWound(g.playerWoundChance()) {
		g.woundCreature(m)
	}
}

// playerWoundChance is the wounding chance of what the player hits with: a
// shapeshifted form's natural weapon, else the wielded one (bare fists never
// wound).
func (g *Game) playerWoundChance() int {
	if g.Shape != nil {
		return g.Shape.Wound
	}
	if g.Weapon != nil && g.Weapon.Def != nil {
		return g.Weapon.Def.Wound
	}
	return 0
}

// killCreature resolves a monster's death: announce it, drop its corpse (when
// the def names one), and remove it from the level. Centralising death here
// keeps every kill site (player now, hazards later) dropping corpses consistently.
func (g *Game) killCreature(m *Creature) {
	g.log("The %s dies.", m.Def.Name)
	g.dropCorpseAndRemove(m)
}

// dropCorpseAndRemove is the silent half of a death: the corpse (when the def
// names one) and the removal.
func (g *Game) dropCorpseAndRemove(m *Creature) {
	g.Sound("death")
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
	g.Sound("zap")
	g.identify(it) // zapping a wand reveals its type
	if m := g.Level.CreatureAt(target); m != nil {
		g.revealMimic(m) // a bolt tears the glamour (C reveal_mimic)
		if it.Def.Damage != "" {
			dmg := g.RNG.RollSpec(it.Def.Damage)
			m.HP -= dmg
			g.log("The %s blasts the %s for %d.", it.Def.Name, m.Def.Name, dmg)
			g.Sound("hit")
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
	if !g.Dead && g.rollWound(m.Def.Wound) {
		g.woundPlayer()
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
	g.Sound("hurt")
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
