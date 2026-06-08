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
	defaultSpeed = 100 // energy per turn for a monster with no speed set
)

// speedOf is a monster's energy gain per turn (a non-positive def speed means
// "average", matching the player's one action per turn).
func speedOf(m *Creature) int {
	if m.Def.Speed > 0 {
		return m.Def.Speed
	}
	return defaultSpeed
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
		if g.Level.At(g.Player).Def.Win {
			g.Won = true
			g.log("You ascend to demigodhood. You win!")
			return // winning ends the turn immediately
		}
	}
	if acted { // a blocked move into a wall doesn't pass the turn
		g.monstersAct()
	}
}

func (g *Game) playerAttackStat() int {
	if g.Weapon != nil {
		return playerAttack + g.Weapon.Def.Attack
	}
	return playerAttack
}

func (g *Game) playerDamageSpec() string {
	if g.Weapon != nil && g.Weapon.Def.Damage != "" {
		return g.Weapon.Def.Damage
	}
	return playerDamage
}

func (g *Game) playerDodgeStat() int {
	if g.Armor != nil {
		return playerDodge + g.Armor.Def.Dodge
	}
	return playerDodge
}

func (g *Game) playerAttacks(m *Creature) {
	if !g.RNG.Chance(g.playerAttackStat(), m.Def.Dodge) {
		g.log("You miss the %s.", m.Def.Name)
		return
	}
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

func (g *Game) monsterAttacks(m *Creature) {
	if !g.RNG.Chance(m.Def.Attack, g.playerDodgeStat()) {
		g.log("The %s misses you.", m.Def.Name)
		return
	}
	dmg := g.RNG.RollSpec(m.Def.Damage)
	g.PlayerHP -= dmg
	g.log("The %s hits you for %d.", m.Def.Name, dmg)
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.DeathCause = m.Def.Name
		g.log("You die.")
	}
}

// monstersAct advances the world after the player's turn. Each living monster
// gains energy equal to its speed and acts for every full turn's worth it
// holds, so faster monsters act more often (the C's move_counter / TURN_TIME
// model). Leftover energy carries to the next turn.
func (g *Game) monstersAct() {
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
		m.Energy += speedOf(m)
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
	if chebyshev(m.Pos, g.Player) == 1 {
		g.monsterAttacks(m)
		return
	}
	if chebyshev(m.Pos, g.Player) <= senseRange {
		g.stepToward(m, g.Player)
	}
}

func (g *Game) stepToward(m *Creature, target Pos) {
	step := func(a, b int) int {
		switch {
		case b > a:
			return 1
		case b < a:
			return -1
		default:
			return 0
		}
	}
	dst := Pos{m.Pos.X + step(m.Pos.X, target.X), m.Pos.Y + step(m.Pos.Y, target.Y)}
	if dst == g.Player || !g.Level.Passable(dst) || g.Level.CreatureAt(dst) != nil {
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
